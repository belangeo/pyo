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
from math import pow, log10

######################################################################
### Map -> rescale values from sliders
######################################################################
class Map:
    """
    Converts value between 0 and 1 on various scales.
    
    Base class for Map objects.
    
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

    Examples:
    
    >>> m = Map(20., 20000., 'log')
    >>> print m.get(.5)
    632.455532034
    >>> print m.set(12000)
    0.926050416795
    
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
        Takes `x` in the real range and returns value unscaled 
        (between 0 and 1).
        
        """
        
        if self._scale == 'log':
            return log10(x/self._min) / log10(self._max/self._min)
        else:
            return (x - self._min) / (self._max - self._min)

    @property
    def min(self): return self._min
    @property
    def max(self): return self._max
    @property
    def scale(self): return self._scale

class SLMap(Map):
    """
    Base Map class used to manage control sliders. 
    
    Derived from Map class, a few parameters are added for sliders 
    initialization.
    
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
        Ramp time, in seconds, used to smooth the signal sent from slider 
        to object's attribute. Defaults to 0.025.

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
    
    Examples:
    
    >>> s = Server().boot()
    >>> initvals = [350,360,375,388]
    >>> maps = [SLMap(20., 2000., 'log', 'freq', initvals), SLMapMul(.2)]
    >>> a = Sine(freq=initvals, mul=.2).out()
    >>> a.ctrl(maps)  
    >>> s.gui(locals())      

    """
    def __init__(self, min, max, scale, name, init, res='float', ramp=0.025):
        Map.__init__(self, min, max, scale)
        self._name, self._init, self._res, self._ramp = name, init, res, ramp

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

    Parent class: SLMap
    
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

class SLMapMul(SLMap):
    """
    SLMap with normalized values for a 'mul' slider.

    Parent class: SLMap
    
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

class SLMapPhase(SLMap):
    """
    SLMap with normalized values for a 'phase' slider.

    Parent class: SLMap
    
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

class SLMapPan(SLMap):
    """
    SLMap with normalized values for a 'pan' slider.

    Parent class: SLMap
    
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

class SLMapQ(SLMap):
    """
    SLMap with normalized values for a 'q' slider.

    Parent class: SLMap
    
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

class SLMapDur(SLMap):
    """
    SLMap with normalized values for a 'dur' slider.

    Parent class: SLMap
    
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
