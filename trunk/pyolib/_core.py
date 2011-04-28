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
from types import ListType, SliceType, FloatType, StringType
import random, os, sys, inspect, tempfile
from subprocess import call
from distutils.sysconfig import get_python_lib

PYO_VERSION = '0.4.0'

import __builtin__
if hasattr(__builtin__, 'pyo_use_double'):
    from _pyo64 import *
    print "pyo version %s (uses double precision)" % PYO_VERSION
else:    
    from _pyo import *
    print "pyo version %s (uses single precision)" % PYO_VERSION
    
from _maps import *
from _widgets import createCtrlWindow, createViewTableWindow, createViewMatrixWindow

######################################################################
### Utilities
######################################################################
SNDS_PATH = os.path.join(get_python_lib(), "pyolib", "snds")
XNOISE_DICT = {'uniform': 0, 'linear_min': 1, 'linear_max': 2, 'triangle': 3, 
                'expon_min': 4, 'expon_max': 5, 'biexpon': 6, 'cauchy': 7, 
                'weibull': 8, 'gaussian': 9, 'poisson': 10, 'walker': 11, 
                'loopseg': 12}
        
def convertArgsToLists(*args):
    """
    Convert all arguments to list if not already a list or a PyoObject. 
    Return new args and maximum list length.
    
    """
    first = True
    for i in args:
        if isinstance(i, PyoObject): pass  
        elif isinstance(i, PyoTableObject): pass 
        elif isinstance(i, PyoMatrixObject): pass 
        elif type(i) != ListType: i = [i]
            
        if first: tup = (i,)
        else: tup = tup + (i,)
        
        first = False
        
    lengths = [len(i) for i in tup]
    max_length = max(lengths)
    tup = tup + (max_length, )  
    return tup

def wrap(arg, i):
    """
    Return value at position `i` from `arg` with wrap around `arg` length.
    
    """    
    x = arg[i % len(arg)]
    if isinstance(x, PyoObject):
        return x[0]
    elif isinstance(x, PyoTableObject):
        return x[0]
    elif isinstance(x, PyoMatrixObject):
        return x[0]
    else:
        return x

if sys.version_info[:2] <= (2, 5):
    def example(cls, dur=5):
        """
        Runs the example given in the __doc__ string of the object in argument.
    
        example(cls, dur=5)
    
        Parameters:
    
        cls : PyoObject class
            Class reference of the desired object example.
        dur : float, optional
            Duration of the example.
        
        """
        doc = cls.__doc__.split("Examples:")[1]
        lines = doc.splitlines()
        ex_lines = [line.lstrip("    ") for line in lines if ">>>" in line or "..." in line]
        if hasattr(__builtin__, 'pyo_use_double'):
            ex = "import time\nfrom pyo64 import *\n"
        else:
            ex = "import time\nfrom pyo import *\n"
        for line in ex_lines:
            if ">>>" in line: line = line.lstrip(">>> ")
            if "..." in line: line = "    " +  line.lstrip("... ")
            ex += line + "\n"
        ex += "time.sleep(%f)\ns.stop()\ntime.sleep(0.25)\ns.shutdown()\n" % dur
        f = open('/tmp/pyo_example.py', 'w')
        f.write('print """\n%s\n"""\n' % ex)
        f.write(ex)
        f.close()    
        p = call(["python", '/tmp/pyo_example.py'])
else:
    def example(cls, dur=5):
        """
        Runs the example given in the __doc__ string of the object in argument.
    
        example(cls, dur=5)
    
        Parameters:
    
        cls : PyoObject class
            Class reference of the desired object example.
        dur : float, optional
            Duration of the example.
        
        """
        doc = cls.__doc__.split("Examples:")[1]
        lines = doc.splitlines()
        ex_lines = [line.lstrip("    ") for line in lines if ">>>" in line or "..." in line]
        if hasattr(__builtin__, 'pyo_use_double'):
            ex = "import time\nfrom pyo64 import *\n"
        else:
            ex = "import time\nfrom pyo import *\n"
        for line in ex_lines:
            if ">>>" in line: line = line.lstrip(">>> ")
            if "..." in line: line = "    " +  line.lstrip("... ")
            ex += line + "\n"
        ex += "time.sleep(%f)\ns.stop()\ntime.sleep(0.25)\ns.shutdown()\n" % dur
        f = tempfile.NamedTemporaryFile(delete=False)
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
    in input and returns the init line of that class with the default values.
    
    Parameters:
    
    cls : PyoObject class
        Class reference of the desired object init line.

    """
    name = cls.__name__
    arg, varargs, varkw, defaults = inspect.getargspec(getattr(cls, "__init__"))
    arg = inspect.formatargspec(arg, varargs, varkw, defaults, formatvalue=removeExtraDecimals)
    arg = arg.replace("self, ", "")
    return name + arg
        
######################################################################
### PyoObject -> base class for pyo sound objects
######################################################################
class PyoObject(object):
    """
    Base class for all pyo objects that manipulate vectors of samples.
    
    The user should never instantiate an object of this class.

    Methods:

    play(dur, delay) : Start processing without sending samples to output. 
        This method is called automatically at the object creation.
    stop() : Stop processing.
    out(chnl, inc, dur, delay) : Start processing and send samples to audio 
        output beginning at `chnl`.
    mix(voices) : Mix object's audio streams into `voices` streams and 
        return the Mix object.
    setMul(x) : Replace the `mul` attribute.
    setAdd(x) : Replace the `add` attribute.
    setDiv(x) : Replace and inverse the `mul` attribute.
    setSub(x) : Replace and inverse the `add` attribute.
    set(attr, value, port) : Replace any attribute with portamento. 
    ctrl(map_list, title) : Opens a sliders window to control parameters.
    get(all) : Return the first sample of the current buffer as a float.
    dump() : Print current status of the object's attributes.

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
    
    """
    def __init__(self):
        self._target_dict = {}
        self._signal_dict = {}
        self._keep_trace = []
        self._mul = 1.0
        self._add = 0.0
        self._add_dummy = None
        self._mul_dummy = None

    def __add__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._add_dummy = Dummy([obj + wrap(x,i) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                self._add_dummy = x + self 
            else:
                self._add_dummy = Dummy([wrap(self._base_objs,i) + obj for i, obj in enumerate(x)])  
        return self._add_dummy
        
    def __radd__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._add_dummy = Dummy([obj + wrap(x,i) for i, obj in enumerate(self._base_objs)])
        else:
            self._add_dummy = Dummy([wrap(self._base_objs,i) + obj for i, obj in enumerate(x)])                
        return self._add_dummy
            
    def __iadd__(self, x):
        self.setAdd(x)
        return self

    def __sub__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._add_dummy = Dummy([obj - wrap(x,i) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                print 'Substraction Warning: %s - %s' % (self.__repr__(), x.__repr__()),
                print 'Right operator trunctaded to match left operator number of streams.'
                self._add_dummy = Dummy([obj - wrap(x,i) for i, obj in enumerate(self._base_objs)])
            else:
                self._add_dummy = Dummy([wrap(self._base_objs,i) - obj for i, obj in enumerate(x)])
        return self._add_dummy

    def __rsub__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._add_dummy = Dummy([Sig(wrap(x,i)) - obj for i, obj in enumerate(self._base_objs)])
        else:
            self._add_dummy = Dummy([Sig(obj) - wrap(self._base_objs,i) for i, obj in enumerate(x)])
        return self._add_dummy

    def __isub__(self, x):
        self.setSub(x)
        return self
 
    def __mul__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._mul_dummy = Dummy([obj * wrap(x,i) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                self._mul_dummy = x * self 
            else:
                self._mul_dummy = Dummy([wrap(self._base_objs,i) * obj for i, obj in enumerate(x)])  
        return self._mul_dummy
        
    def __rmul__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._mul_dummy = Dummy([obj * wrap(x,i) for i, obj in enumerate(self._base_objs)])
        else:
            self._mul_dummy = Dummy([wrap(self._base_objs,i) * obj for i, obj in enumerate(x)])                
        return self._mul_dummy
            
    def __imul__(self, x):
        self.setMul(x)
        return self
 
    def __div__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._mul_dummy = Dummy([obj / wrap(x,i) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                print 'Division Warning: %s / %s' % (self.__repr__(), x.__repr__()),
                print 'Right operator trunctaded to match left operator number of streams.'
                self._mul_dummy = Dummy([obj / wrap(x,i) for i, obj in enumerate(self._base_objs)])
            else:
                self._mul_dummy = Dummy([wrap(self._base_objs,i) / obj for i, obj in enumerate(x)])
        return self._mul_dummy

    def __rdiv__(self, x):
        self._keep_trace.append(x)
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            self._mul_dummy = Dummy([Sig(wrap(x,i)) / obj for i, obj in enumerate(self._base_objs)])
        else:
            self._mul_dummy = Dummy([Sig(obj) / wrap(self._base_objs,i) for i, obj in enumerate(x)])
        return self._mul_dummy

    def __idiv__(self, x):
        self.setDiv(x)
        return self
        
    def __getitem__(self, i):
        if type(i) == SliceType or i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
 
    def __len__(self):
        return len(self._base_objs)

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj

        if hasattr(self, "_input"):
            if type(self._input) == ListType:
                for pyoObj in self._input:
                    if hasattr(pyoObj, "getBaseObjects"):
                        for obj in pyoObj.getBaseObjects():
                            obj.deleteStream()
                            del obj
            else:
                if hasattr(self._input, "getBaseObjects"):
                    for obj in self._input.getBaseObjects():
                        obj.deleteStream()
                        del obj
            del self._input

        for key in self.__dict__.keys():
            if isinstance(self.__dict__[key], PyoObject):
                del self.__dict__[key]
            elif type(self.__dict__[key]) == ListType:
                for ele in self.__dict__[key]:
                    try:
                        ele.deleteStreams()
                        del ele
                    except:
                        del ele

        if hasattr(self, "_in_fader"):
            del self._in_fader

    def __repr__(self):
        return '< Instance of %s class >' % self.__class__.__name__
        
    def dump(self):
        """
        Print the number of streams and the current status of the 
        object's attributes.
        
        """
        attrs = dir(self)
        pp =  '< Instance of %s class >' % self.__class__.__name__
        pp += '\n-----------------------------'
        pp += '\nNumber of audio streams: %d' % len(self)
        pp += '\n--- Attributes ---'
        for attr in attrs:
            pp += '\n' + attr + ': ' + str(getattr(self, attr))
        pp += '\n-----------------------------'
        return pp    
            
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
            
    def getBaseObjects(self):
        """
        Return a list of audio Stream objects.
        
        """
        return self._base_objs
        
    def play(self, dur=0, delay=0):
        """
        Start processing without sending samples to output. 
        This method is called automatically at the object creation.
        
        Parameters:
        
        dur : float, optional
            Duration, in seconds, of the object's activation. The default is 0
            and means infinite duration.
        delay : float, optional
            Delay, in seconds, before the object's activation. Defaults to 0.
        
        """
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        """
        Start processing and send samples to audio output beginning at `chnl`.
        
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
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        """
        Stop processing.
        
        """
        [obj.stop() for obj in self._base_objs]
        return self

    def mix(self, voices=1):
        """
        Mix the object's audio streams into `voices` streams and return 
        the Mix object.
        
        Parameters:

        voices : int, optional
            Number of audio streams of the Mix object created by this method. 
            If more than 1, object's streams are alternated and added into 
            Mix object's streams. Defaults to 1.
            
        """
        return Mix(self, voices)
        #return self._mix
        
    def setMul(self, x):
        """
        Replace the `mul` attribute.
        
        Parameters:

        x : float or PyoObject
            New `mul` attribute.
        
        """
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setMul(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setAdd(self, x):
        """
        Replace the `add` attribute.
                
        Parameters:

        x : float or PyoObject
            New `add` attribute.
        
        """
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setAdd(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSub(self, x):
        """
        Replace and inverse the `mul` attribute.
        
        Parameters:

        x : float or PyoObject
            New inversed `mul` attribute.
        
        """
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setSub(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDiv(self, x):
        """
        Replace and inverse the `add` attribute.
                
        Parameters:

        x : float or PyoObject
            New inversed `add` attribute.
        
        """
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setDiv(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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
        self._signal_dict[attr] = VarPort(value, port, getattr(self, attr), self._reset_from_set, attr)
        setattr(self, attr, self._signal_dict[attr])

    def _reset_from_set(self, attr=None):
        setattr(self, attr, self._target_dict[attr])
        self._signal_dict[attr].stop()
        del self._signal_dict[attr]
        
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
class PyoTableObject(object):
    """
    Base class for all pyo table objects. 
    
    A table object is a buffer memory to store precomputed samples. 
    
    The user should never instantiate an object of this class.
 
    Methods:
    
    getSize() : Return table size in samples.
    view() : Opens a window showing the contents of the table.
    dump() : Print current status of the object's attributes.
    save(path, format) : Writes the content of the table in an audio file.
    write(path, oneline) : Writes the content of the table in a text file.
    read(path) : Sets the content of the table from a text file.
    normalize() : Normalize table samples between -1 and 1.
    put(value, pos) : Puts a value at specified position in the table.
    get(pos) : Returns the value at specified position in the table.
    
    Notes:
    
    Operations allowed on all table objects :
    
    len(obj) : Return the number of table streams in an object.
    obj[x] : Return table stream `x` of the object. `x` is a number 
        from 0 to len(obj) - 1.

    """
    def __init__(self):
        pass

    def __getitem__(self, i):
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
 
    def __len__(self):
        return len(self._base_objs)

    def __repr__(self):
        return '< Instance of %s class >' % self.__class__.__name__
        
    def dump(self):
        """
        Print the number of streams and the current status of the 
        object's attributes.
        
        """
        attrs = dir(self)
        pp =  '< Instance of %s class >' % self.__class__.__name__
        pp += '\n-----------------------------'
        pp += '\nNumber of table streams: %d' % len(self)
        pp += '\n--- Attributes ---'
        for attr in attrs:
            pp += '\n' + attr + ': ' + str(getattr(self, attr))
        pp += '\n-----------------------------'
        return pp    

    def save(self, path, format=0):
        """
        Writes the content of the table in an audio file.
        
        The sampling rate of the file is the sampling rate of the server
        and the number of channels is the number of table streams of the
        object.

        Parameters:
        
        path : string
            Full path (including extension) of the new file.
        format : int, optional
            Format type of the file. Possible formats are:
                0 : AIFF 32 bits float (Default)
                1 : WAV 32 bits float
                2 : AIFF 16 bits int
                3 : WAV 16 bits int
                4 : AIFF 24 bits int
                5 : WAV 24 bits int
                6 : AIFF 32 bits int
                7 : WAV 32 bits int

        """
        sr = int(self._base_objs[0].getServer().getSamplingRate())
        if len(self._base_objs) == 1:
            samples = self._base_objs[0].getTable()
        else:
            samples = [obj.getTable() for i, obj in enumerate(self._base_objs)]
        savefile(samples, path, sr, len(self._base_objs), format)    
    
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
        
    def getBaseObjects(self):
        """
        Return a list of table Stream objects.
        
        """
        return self._base_objs

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

    def normalize(self):
        """
        Normalize table samples between -1 and 1.

        """
        [obj.normalize() for obj in self._base_objs]
        return self

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
        createViewTableWindow(samples, title, wxnoserver, self.__class__.__name__)
        
######################################################################
### PyoMatrixObject -> base class for pyo matrix objects
######################################################################
class PyoMatrixObject(object):
    """
    Base class for all pyo matrix objects. 
    
    A matrix object is a 2 dimensions buffer memory to store 
    precomputed samples. 
    
    The user should never instantiate an object of this class.
 
    Methods:
    
    getSize() : Return matrix size in samples (x, y).
    view() : Opens a window showing the contents of the matrix.
    dump() : Print current status of the object's attributes.
    write(path) : Writes the content of the matrix in a text file.
    read(path) : Sets the content of the matrix from a text file.
    normalize() : Normalize matrix samples between -1 and 1.
    blur() : Apply a simple gaussian blur on the matrix.
    boost(min, max, boost) : Boost the contrast of values in the matrix.
    put(value, x, y) : Puts a value at specified position in the matrix.
    get(x, y) : Returns the value at specified position in the matrix.
    
    Notes:
    
    Operations allowed on all matrix objects :
    
    len(obj) : Return the number of table streams in an object.
    obj[x] : Return table stream `x` of the object. `x` is a number 
        from 0 to len(obj) - 1.

    """
    def __init__(self):
        pass

    def __getitem__(self, i):
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
 
    def __len__(self):
        return len(self._base_objs)

    def __repr__(self):
        return '< Instance of %s class >' % self.__class__.__name__
        
    def dump(self):
        """
        Print the number of streams and the current status of the 
        object's attributes.
        
        """
        attrs = dir(self)
        pp =  '< Instance of %s class >' % self.__class__.__name__
        pp += '\n-----------------------------'
        pp += '\nNumber of matrix streams: %d' % len(self)
        pp += '\n--- Attributes ---'
        for attr in attrs:
            pp += '\n' + attr + ': ' + str(getattr(self, attr))
        pp += '\n-----------------------------'
        return pp    

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
        
    def getBaseObjects(self):
        """
        Returns a list of matrix stream objects.
        
        """
        return self._base_objs

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
        createViewMatrixWindow(samples, self.getSize(), title, wxnoserver)
        
######################################################################
### Internal classes -> Used by pyo
######################################################################
class Mix(PyoObject):
    """
    Mix audio streams to arbitrary number of streams.
    
    Mix the object's audio streams in `ìnput` into `voices` streams.
    
    Parent class: PyoObject

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
    >>> a = Sine([random.uniform(400,600) for i in range(50)], mul=.01)
    >>> b = Mix(a).out()
    >>> print len(a)
    50
    >>> print len(b)
    1

    """
    def __init__(self, input, voices=1, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
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

    def __dir__(self):
        return ['mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        del self._base_objs
        if type(self._input) == ListType:
            for pyoObj in self._input:
                for obj in pyoObj.getBaseObjects():
                    obj.deleteStream()
                    del obj
        else:
            for obj in self._input.getBaseObjects():
                obj.deleteStream()
                del obj
        del self._input

class Dummy(PyoObject):
    """
    Dummy object used to perform arithmetics on PyoObject.
    
    The user should never instantiate an object of this class.
    
    Parent class: PyoObject

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
    >>> m = Metro().play()
    >>> p = TrigRand(m, 250, 400)
    >>> a = Sine(p, mul=.25).out()
    >>> b = Sine(p*1.25, mul=.25).out()
    >>> c = Sine(p*1.5, mul=.25).out()
    
    """
    def __init__(self, objs_list):
        PyoObject.__init__(self)
        self._mul = 1
        self._add = 0
        self._base_objs = objs_list

    def __dir__(self):
        return ['mul', 'add']

    def deleteStream(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        
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
    
    The setInput method, on object with `input` attribute, uses 
    an InputFader object to performs crossfade between the old and the 
    new `input` of an object. 
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(450, mul=.5)
    >>> b = Sine(650, mul=.5)
    >>> c = InputFader(a).out()
    >>> # to created a crossfade, calls:
    >>> c.setInput(b, 20)
    
    """
    def __init__(self, input):
        PyoObject.__init__(self)
        self._input = input
        input, lmax = convertArgsToLists(input)
        self._base_objs = [InputFader_base(wrap(input,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'mul', 'add']

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
    
    Parent class: PyoObject

    Parameters:

    value : float or PyoObject
        Numerical value to convert.

    Methods:

    setValue(x) : Changes the value of the signal stream.
    
    Attributes:
    
    value : float or PyoObject. Numerical value to convert.

    Notes:

    The out() method is bypassed. Sig's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> fr = Sig(value=400)
    >>> p = Port(fr, risetime=1, falltime=1)
    >>> a = Sine(freq=p, mul=.5).out()
    >>> s.start()
    >>> fr.value = 800

    """
    def __init__(self, value, mul=1, add=0):
        PyoObject.__init__(self)
        self._value = value
        self._mul = mul
        self._add = add
        value, mul ,add, lmax = convertArgsToLists(value, mul, add)
        self._base_objs = [Sig_base(wrap(value,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['value', 'mul', 'add']

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
    current value to the new value. If a callback is provided at `function`,
    it will be called at the end of the line.

    Parent class: PyoObject

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
    .... 
    >>> fr = VarPort(value=800, time=2, init=400, function=callback, arg="YEP!")
    >>> a = Sine(freq=fr, mul=.5).out()

    """
    def __init__(self, value, time=0.025, init=0.0, function=None, arg=None, mul=1, add=0):
        PyoObject.__init__(self)
        self._value = value
        self._time = time
        self._mul = mul
        self._add = add
        value, time, init, function, arg, mul ,add, lmax = convertArgsToLists(value, time, init, function, arg, mul, add)
        self._base_objs = [VarPort_base(wrap(value,i), wrap(time,i), wrap(init,i), wrap(function,i), wrap(arg,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['value', 'time', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
