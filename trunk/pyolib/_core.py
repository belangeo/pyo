# -*- coding: utf-8 -*-
from types import ListType
import random, threading, time, os
from math import pow, log10
from distutils.sysconfig import get_python_lib

import Tkinter
from Tkinter import *
Tkinter.NoDefaultRoot()

from _pyo import *

######################################################################
### Utilities
######################################################################
DEMOS_PATH = os.path.join(get_python_lib(), "pyodemos")

class Call_example:
    def __init__(self, callback):
        self.__call__ = callback

class Print_args:
    def __init__(self, callback):
        self.__call__ = callback

class Clean_objects(threading.Thread):
    def __init__(self, time, *args):
        threading.Thread.__init__(self)
        self.t = time
        self.args = args
        
    def run(self):
        time.sleep(self.t)
        for arg in self.args:
            try: arg.stop()
            except: pass
        for arg in self.args:
            del arg 
        
def convertArgsToLists(*args):
    """
    Convert all arguments to list if not already a list or a PyoObject. 
    Return new args and maximum list length.
    
    """
    first = True
    for i in args:
        if isinstance(i, PyoObject): pass  
        elif isinstance(i, PyoTableObject): pass 
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
    else:
        return x

######################################################################
### Map -> rescale values from sliders
######################################################################
class Map:
    """
    Base class for Map objects. Can be used to convert value between 0 and 1
    on various scales.
    
    Parameters:
    
    min : int or float
        Lowest value of the range.
    max : int or float
        Highest value of the range.
    scale : string {'lin', 'log'}
        Method used to scale the input value on the specified range.
        
    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    Attributes:
    
    min : Lowest value of the range.
    max : Highest value of the range.
    scale : Method used to scale the input value.
    
    """
    def __init__(self, min, max, scale):
        self._min, self._max, self._scale = min, max, scale

    def get(self, x):
        """
        Takes `x` between 0 and 1 and returns scaled value.
        
        """
        if x < 0: x = 0.0
        elif x > 1: x = 1.0 
        
        if self._scale == 'log':
            return pow(10, x * log10(self._max/self._min) + log10(self._min))
        else:
            return (self._max - self._min) * x + self._min

    def set(self, x):
        """
        Takes `x` in the real range and returns value unscaled (between 0 and 1).
        
        """
        
        if self._scale == 'log':
            return log10(x/self._min) / log10(self._max/self._min)
        else:
            return (x - self._min) / (self._max - self._min)

    def args():
        return("Map(min, max, scale)")
    args = Print_args(args)

    @property
    def min(self): return self._min
    @property
    def max(self): return self._max
    @property
    def scale(self): return self._scale

class SLMap(Map):
    """
    
    Base Map class used to manage control sliders. Derived from Map class,
    a few parameters are added for sliders initialization.
    
    Parent class: Map
    
    Parameters:

    min : int or float
        Smallest value of the range.
    max : int or float
        Highest value of the range.
    scale : string {'lin', 'log'}
        Method used to scale the input value on the specified range.    
    name : string
        Name of the attributes the slider is affected to.
    init : int or float
        Initial value. Specified in the real range, not between 0 and 1. Use
        the `set` method to retreive the normalized corresponding value.
    res : string {'int', 'float'}, optional
        Sets the resolution of the slider. Defaults to 'float'.
    ramp : float, optional
        Ramp time, in seconds, used to smooth the signal sent from slider to object's attribute.
        Defaults to 0.025.

    Methods:
    
    get(x) : Returns the scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    Attributes:
    
    min : Lowest value of the range.
    max : Highest value of the range.
    scale : Method used to scale the input value.
    name : Name of the parameter to control.
    init : Initial value of the slider.
    res : Slider resolution {int or float}.
    ramp : Ramp time in seconds.
    
    """
    def __init__(self, min, max, scale, name, init, res='float', ramp=0.025):
        Map.__init__(self, min, max, scale)
        self._name, self._init, self._res, self._ramp = name, init, res, ramp

    def args():
        return("SLMap(min, max, scale, name, init, res='float', ramp=0.025)")
    args = Print_args(args)

    @property
    def name(self): return self._name
    @property
    def init(self): return self._init
    @property
    def res(self): return self._res
    @property
    def ramp(self): return self._ramp
    
class SLMapFreq(SLMap):
    """
    SLMap with normalized values for a 'freq' slider.

    Parent class: SlMap
    
    Parameters:
    
    init : int or float, optional
        Initial value. Specified in the real range, not between 0 and 1.
        Defaults to 1000.

    SLMapFreq values are: 
        
    min = 20.0
    max = 20000.0
    scale = 'log'
    name = 'freq'
    res = 'float'
    ramp = 0.025

    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    """
    def __init__(self, init=1000):
        SLMap.__init__(self, 20., 20000., 'log', 'freq', init, 'float', 0.025)

    def args():
        return("SLMapFreq(init=1000)")
    args = Print_args(args)

class SLMapMul(SLMap):
    """
    SLMap with normalized values for a 'mul' slider.

    Parent class: SlMap
    
    Parameters:
    
    init : int or float, optional
        Initial value. Specified in the real range, not between 0 and 1.
        Defaults to 1.

    SLMapMul values are: 
        
    min = 0.0
    max = 2.0
    scale = 'lin'
    name = 'mul'
    res = 'float'
    ramp = 0.025

    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    """
    def __init__(self, init=1.):
        SLMap.__init__(self, 0., 2., 'lin', 'mul', init, 'float', 0.025)

    def args():
        return("SLMapMul(init=1.)")
    args = Print_args(args)

class SLMapPhase(SLMap):
    """
    SLMap with normalized values for a 'phase' slider.

    Parent class: SlMap
    
    Parameters:
    
    init : int or float, optional
        Initial value. Specified in the real range, not between 0 and 1.
        Defaults to 0.

    SLMapPhase values are: 
        
    min = 0.0
    max = 1.0
    scale = 'lin'
    name = 'phase'
    res = 'float'
    ramp = 0.025

    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    """
    def __init__(self, init=0.):
        SLMap.__init__(self, 0., 1., 'lin', 'phase', init, 'float', 0.025)

    def args():
        return("SLMapPhase(init=0.)")
    args = Print_args(args)

class SLMapPan(SLMap):
    """
    SLMap with normalized values for a 'pan' slider.

    Parent class: SlMap
    
    Parameters:
    
    init : int or float, optional
        Initial value. Specified in the real range, not between 0 and 1.
        Defaults to 0.

    SLMapPhase values are: 
        
    min = 0.0
    max = 1.0
    scale = 'lin'
    name = 'pan'
    res = 'float'
    ramp = 0.025

    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    """
    def __init__(self, init=0.):
        SLMap.__init__(self, 0., 1., 'lin', 'pan', init, 'float', 0.025)

    def args():
        return("SLMapPan(init=0.)")
    args = Print_args(args)

class SLMapQ(SLMap):
    """
    SLMap with normalized values for a 'q' slider.

    Parent class: SlMap
    
    Parameters:
    
    init : int or float, optional
        Initial value. Specified in the real range, not between 0 and 1.
        Defaults to 1.

    SLMapQ values are: 
        
    min = 0.1
    max = 100.0
    scale = 'log'
    name = 'q'
    res = 'float'
    ramp = 0.025

    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    """
    def __init__(self, init=1.):
        SLMap.__init__(self, 0.1, 100., 'log', 'q', init, 'float', 0.025)

    def args():
        return("SLMapQ(init=1.)")
    args = Print_args(args)

class SLMapDur(SLMap):
    """
    SLMap with normalized values for a 'dur' slider.

    Parent class: SlMap
    
    Parameters:
    
    init : int or float, optional
        Initial value. Specified in the real range, not between 0 and 1.
        Defaults to 1.

    SLMapDur values are: 
        
    min = 0.
    max = 60.0
    scale = 'lin'
    name = 'dur'
    res = 'float'
    ramp = 0.025

    Methods:
    
    get(x) : Returns scaled value for `x` between 0 and 1.
    set(x) : Returns the normalized value (0 -> 1) for `x` in the real range.  

    """
    def __init__(self, init=1.):
        SLMap.__init__(self, 0., 60., 'lin', 'dur', init, 'float', 0.025)

    def args():
        return("SLMapDur(init=1.)")
    args = Print_args(args)

######################################################################
### Multisliders
######################################################################
class MultiSlider(Frame):
    def __init__(self, master, init, key, command): 
        Frame.__init__(self, master, bd=0, relief=FLAT)
        self._values = init
        self._nchnls = len(init)
        self._key = key
        self._command = command
        self._lines = []
        self._height = 16
        self._yoff = 4 # hack for OSX display
        self.canvas = Canvas(self, height=self._height*self._nchnls+1, width=225, relief=FLAT, bd=0, bg="#BCBCAA")
        w = self.canvas.winfo_width()
        for i in range(self._nchnls):
            x = int(self._values[i] * w)
            y = self._height * i + self._yoff
            self._lines.append(self.canvas.create_rectangle(0, y, x, y+self._height-1, width=0, fill="#121212"))
        self.canvas.bind("<Button-1>", self.clicked)
        self.canvas.bind("<Motion>", self.move)
        self.canvas.bind("<Configure>", self.size)
        self.canvas.grid(sticky=E+W)
        self.columnconfigure(0, weight=1)
        self.grid()

    def size(self, event):
        w = self.canvas.winfo_width()
        for i in range(len(self._lines)):
            y = self._height * i + self._yoff
            x = self._values[i] * w
            self.canvas.coords(self._lines[i], 0, y, x, y+self._height-1)
        
    def clicked(self, event):
        self.update(event)
        
    def move(self, event):
        if event.state == 0x0100:
            slide = (event.y - self._yoff) / self._height
            if 0 <= slide < len(self._lines):
                self.update(event)

    def update(self, event):
        w = self.canvas.winfo_width()
        slide = (event.y - self._yoff) / self._height
        val = event.x / float(w)
        self._values[slide] = val
        y = self._height * slide + self._yoff
        self.canvas.coords(self._lines[slide], 0, y, event.x, y+self._height-1)
        self._command(self._key, self._values)
           
######################################################################
### Control window for PyoObject
######################################################################
class Command:
    def __init__(self, func, key):
        self.func = func
        self.key = key

    def __call__(self, value):
        self.func(self.key, value)

class PyoObjectControl(Frame):
    def __init__(self, master=None, obj=None, map_list=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        self.bind('<Destroy>', self._destroy)
        self._obj = obj
        self._map_list = map_list
        self._sliders = []
        self._excluded = []
        self._values = {}
        self._displays = {}
        self._maps = {}
        self._sigs = {}
        for i, m in enumerate(self._map_list):
            key, init = m.name, m.init
            # filters PyoObjects
            if isinstance(init, PyoObject):
                self._excluded.append(key)
            else:    
                self._maps[key] = m
                # label (param name)
                label = Label(self, height=1, width=10, highlightthickness=0, text=key)
                label.grid(row=i, column=0)
                # create and pack slider
                if type(init) != ListType:
                    self._sliders.append(Scale(self, command=Command(self.setval, key),
                                  orient=HORIZONTAL, relief=GROOVE, from_=0., to=1., showvalue=False, 
                                  resolution=.0001, bd=1, length=225, troughcolor="#BCBCAA", width=12))
                    self._sliders[-1].set(m.set(init))
                    disp_height = 1
                else:
                    self._sliders.append(MultiSlider(self, [m.set(x) for x in init], key, self.setval)) 
                    disp_height = len(init)   
                self._sliders[-1].grid(row=i, column=1, sticky=E+W)
                # display of numeric values
                textvar = StringVar(self)
                display = Label(self, height=disp_height, width=10, highlightthickness=0, textvariable=textvar)
                display.grid(row=i, column=2)
                self._displays[key] = textvar
                if type(init) != ListType:
                    self._displays[key].set("%.4f" % init)
                else:
                    self._displays[key].set("\n".join(["%.4f" % i for i in init]))
                # set obj attribute to PyoObject SigTo     
                self._sigs[key] = SigTo(init, .025, init)
                setattr(self._obj, key, self._sigs[key])
        # padding        
        top = self.winfo_toplevel()
        top.rowconfigure(0, weight=1)
        top.columnconfigure(0, weight=1)       
        self.columnconfigure(1, weight=1)
        self.grid(ipadx=15, ipady=15, sticky=E+W)

    def _destroy(self, event):
        for m in self._map_list:
            key = m.name
            if key not in self._excluded:
                setattr(self._obj, key, self._values[key])
                del self._sigs[key]
            
        
    def setval(self, key, x):
        if type(x) != ListType:
            value = self._maps[key].get(float(x))
            self._displays[key].set("%.4f" % value)
        else:    
            value = [self._maps[key].get(float(y)) for y in x] 
            self._displays[key].set("\n".join(["%.4f" % i for i in value]))
            
        self._values[key] = value
        setattr(self._sigs[key], "value", value)
        
######################################################################
### PyoObject -> base class for pyo sound objects
######################################################################
class PyoObject(object):
    """
    Base class for all pyo objects that manipulate vectors of samples.
    
    The user should never instantiate an object of this class.

    Methods:

    play() : Start processing without sending samples to output. This method is called automatically
        at the object creation.
    stop() : Stop processing.
    out(chnl, inc) : Start processing and send samples to audio output beginning at `chnl`.
    mix(voices) : Mix object's audio streams into `voices` streams and return the Mix object.
    setMul(x) : Replace the `mul` attribute.
    setAdd(x) : Replace the `add` attribute.
    setDiv(x) : Replace and inverse the `mul` attribute.
    setSub(x) : Replace and inverse the `add` attribute.
    ctrl(map_list, title) : Opens a sliders window to control object's parameters.

    Attributes:

    mul : float or PyoObject
        Multiplication factor.
    add : float or PyoObject
        Addition factor.
    
    Notes:

    - Other operations:
    
    len(obj) : Return the number of audio streams in an object.
    obj[x] : Return stream `x` of the object. `x` is a number from 0 to len(obj) - 1.
    del obj : Perform a clean delete of the object.
    
    - Arithmetics:
    
    Multiplication, addition, division and substraction can be applied between pyo objects
    or between pyo objects and numbers. Doing so returns a Dummy object with the result of the operation.
    `b = a * 0.5` creates a Dummy object `b` with `mul` attribute set to 0.5 and leave `a` untouched.
    
    Inplace multiplication, addition, division and substraction can be applied between pyo 
    objects or between pyo objects and numbers. These operations will replace the `mul` or `add`
    factor of the object. `a *= 0.5` replaces the `mul` attribute of `a`.
    
    - Class methods:
    
    demo() : This method called on a class (not an instance of that class) will start an interactive
             session showing possible uses of the object.
    args() : This method called on a class (not an instance of that class) returns the init line of
             the object with the default values.
    
    """
    def __init__(self):
        pass

    def __add__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([obj + wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._add_dummy
        
    def __radd__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([obj + wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._add_dummy
            
    def __iadd__(self, x):
        self.setAdd(x)
        return self

    def __sub__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([obj - wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._add_dummy

    def __rsub__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([Sig(wrap(x,i)) - obj for i, obj in enumerate(self._base_objs)])
        return self._add_dummy

    def __isub__(self, x):
        self.setSub(x)
        return self
 
    def __mul__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([obj * wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy
        
    def __rmul__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([obj * wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy
            
    def __imul__(self, x):
        self.setMul(x)
        return self
 
    def __div__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([obj / wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy

    def __rdiv__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([Sig(wrap(x,i)) / obj for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy

    def __idiv__(self, x):
        self.setDiv(x)
        return self
        
    def __getitem__(self, i):
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
 
    def __len__(self):
        return len(self._base_objs)

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        
    def getBaseObjects(self):
        """
        Return a list of audio Stream objects.
        
        """
        return self._base_objs
        
    def play(self):
        """
        Start processing without sending samples to output. This method is called automatically
        at the object creation.
        
        """
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        """
        Start processing and send samples to audio output beginning at `chnl`.
        
        Parameters:

        chnl : int, optional
            Physical output assigned to the first audio stream of the object. Defaults to 0.

            If `chnl` is an integer equal or greater than 0: successive streams 
            increment the output number by `inc` and wrap around the global number of channels.
            
            If `chnl` is a negative integer: streams begin at 0, increment the output 
            number by `inc` and wrap around the global number of channels. Then, the list
            of streams is scrambled.
            
            If `chnl` is a list: successive values in the list will be assigned to successive streams.
            
        inc : int, optional
            Output increment value.
        
        """
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        """
        Stop processing.
        
        """
        [obj.stop() for obj in self._base_objs]
        return self

    def mix(self, voices=1):
        """
        Mix the object's audio streams into `voices` streams and return the Mix object.
        
        Parameters:

        voices : int, optional
            Number of audio streams of the Mix object created by this method. If more
            than 1, object's streams are alternated and added into Mix object's streams. 
            Defaults to 1.
            
        """
        self._mix = Mix(self, voices)
        return self._mix
        
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

    def ctrl(self, map_list=None, title=None):
        """
        Opens a sliders window to control parameters of the object. Only parameters
        that can be set to a PyoObject are allowed to be mapped on a slider.

        If a list of values are given to a parameter, a multisliders will be used to
        control each stream independently.
        
        Parameters:

        map_list : list of SLMap objects, optional
            Users defined set of parameters scaling. There is default scaling for
            each object that accept `ctrl` method.
        title : string, optional
            Title of the window. If none is provided, the name of the class is used.

        """
        pass
        
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
### View window for PyoTableObject
######################################################################
class ViewTable(Frame):
    def __init__(self, master=None, samples=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        self.samples = samples
        self.line_points = []
        self.width = 400
        self.height = 150
        self.half_height = self.height / 2
        self.wave_amp = self.half_height -2
        self.canvas = Canvas(self, height=self.height, width=self.width, relief=SUNKEN, bd=1, bg="#EFEFEF")
        step = len(samples) / float(self.width - 1)
        for i in range(self.width):
            y = self.samples[int(i*step)-1] * self.wave_amp + self.wave_amp + 6
            self.line_points.append(i+4)
            self.line_points.append(y)
            self.line_points.append(i+4)
            self.line_points.append(y)
        self.canvas.create_line(0, self.half_height+4, self.width, self.half_height+4, fill='grey', dash=(4,2))    
        self.canvas.create_line(*self.line_points)
        self.canvas.grid()
        self.grid(ipadx=10, ipady=10)
           
######################################################################
### PyoTableObject -> base class for pyo table objects
######################################################################
class PyoTableObject(object):
    """
    Base class for all pyo table objects. A table object object is a buffer memory
    to store precomputed samples. 
    
    The user should never instantiate an object of this class.
 
    Methods:
    
    getSize() : Return table size in samples.
    
    Notes:
    
    Operations allowed on all table objects :
    
    len(obj) : Return the number of table streams in an object.
    obj[x] : Return table stream `x` of the object. `x` is a number from 0 to len(obj) - 1.

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

    def view(self):
        samples = self._base_objs[0].getTable()
        win = Tk()
        f = ViewTable(win, samples)
        win.resizable(False, False)
        win.title("Table waveform")
       
######################################################################
### Internal classes -> Used by pyo
######################################################################
class Mix(PyoObject):
    def __init__(self, input, voices=1, mul=1, add=0):
        self._input = input
        self._mul = mul
        self._add = add
        input_objs = input.getBaseObjects()
        if voices < 1: voices = 1
        elif voices > len(input_objs): voices = len(input_objs)
        sub_lists = [[]] * voices
        [sub_lists[i % voices].append(obj) for i, obj in enumerate(input_objs)]
        self._base_objs = [Mix_base(l) for l in sub_lists]
        
class Dummy(PyoObject):
    def __init__(self, objs_list):
        self._base_objs = objs_list

class InputFader(PyoObject):
    def __init__(self, input):
        self._input = input
        input, lmax = convertArgsToLists(input)
        self._base_objs = [InputFader_base(wrap(input,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x,i), fadetime) for i, obj in enumerate(self._base_objs)]

class Sig(PyoObject):
    """
    Convert numeric value to PyoObject signal.

    Parameters:

    value : float
        Numerical value to convert.

    Methods:

    setValue(x) : Changes the value of the signal stream.
    
    Attributes:
    
    value : float. Numerical value to convert.
    
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
        self._value = value
        self._mul = mul
        self._add = add
        value, mul ,add, lmax = convertArgsToLists(value, mul, add)
        self._base_objs = [Sig_base(wrap(value,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setValue(self, x):
        """
        Changes the value of the signal stream.

        Parameters:

        x : float
            Numerical value to convert.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control for Sig object."

    #def demo():
    #    execfile("demos/Sig_demo.py")
    #demo = Call_example(demo)

    def args():
        return("Sig(value, mul=1, add=0)")
    args = Print_args(args)
    
    @property
    def value(self):
        """float. Numerical value to convert.""" 
        return self._value
    @value.setter
    def value(self, x): self.setValue(x)    

class SigTo(PyoObject):
    """
    Convert numeric value to PyoObject signal applying a ramp between current
    value and new value.

    Parameters:

    value : float
        Numerical value to convert.
    time : float, optional
        Ramp time, in seconds, to reach the new value. Defaults to 0.025.
    init : float, optional
        Initial value of the internal memory. Defaults to 0.

    Methods:

    setValue(x) : Changes the value of the signal stream.
    setTime(x) : Changes the ramp time.
    
    Attributes:
    
    value : float. Numerical value to convert.
    time : float. Ramp time.
    
    Notes:

    The out() method is bypassed. Sig's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> fr = SigTo(value=400, time=.5, init=400)
    >>> a = Sine(freq=fr, mul=.5).out()
    >>> s.start()
    >>> fr.value = 800

    """
    def __init__(self, value, time=0.025, init=0.0, mul=1, add=0):
        self._value = value
        self._time = time
        self._mul = mul
        self._add = add
        value, time, init, mul ,add, lmax = convertArgsToLists(value, time, init, mul, add)
        self._base_objs = [SigTo_base(wrap(value,i), wrap(time,i), wrap(init,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setValue(self, x):
        """
        Changes the value of the signal stream.

        Parameters:

        x : float
            Numerical value to convert.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        """
        Changes the ramp time of the object.

        Parameters:

        x : float
            New ramp time.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control for SigTo object."

    #def demo():
    #    execfile("demos/SigTo_demo.py")
    #demo = Call_example(demo)

    def args():
        return("SigTo(value, time=0.025, init=0.0, mul=1, add=0)")
    args = Print_args(args)
    
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
