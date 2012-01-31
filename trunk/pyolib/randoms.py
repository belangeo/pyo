"""
Random noise generators.

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
from types import StringType, ListType

class Randi(PyoObject):
    """
    Periodic pseudo-random generator with interpolation.
    
    Randi generates a pseudo-random number between `min` and `max` 
    values at a frequency specified by `freq` parameter. Randi will 
    produce straight-line interpolation between current number and the next.
    
    Parentclass: PyoObject

    Parameters:

    min : float or PyoObject, optional
        Minimum value for the random generation. Defaults to 0.
    max : float or PyoObject, optional
        Maximum value for the random generation. Defaults to 1.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    
    Methods:

    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:
    
    min : float or PyoObject. Minimum value.
    max : float or PyoObject. Maximum value.
    freq : float or PyoObject. Polling frequency.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> rnd = Randi(400, 600, 4)
    >>> a = Sine(rnd, mul=.5).out()
    
    """
    def __init__(self, min=0., max=1., freq=1., mul=1, add=0):
        PyoObject.__init__(self)
        self._min = min
        self._max = max
        self._freq = freq
        self._mul = mul
        self._add = add
        min, max, freq, mul, add, lmax = convertArgsToLists(min, max, freq, mul, add)
        self._base_objs = [Randi_base(wrap(min,i), wrap(max,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['min', 'max', 'freq', 'mul', 'add']
  
    def setMin(self, x):
        """
        Replace the `min` attribute.
        
        Parameters:

        x : float or PyoObject
            new `min` attribute.
        
        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.
        
        Parameters:

        x : float or PyoObject
            new `max` attribute.
        
        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            new `freq` attribute.
        
        """
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(1., 2., 'lin', 'max', self._max),
                          SLMap(0.1, 20., 'lin', 'freq', self._freq),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def min(self): return self._min
    @min.setter
    def min(self, x): self.setMin(x)
    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

class Randh(PyoObject):
    """
    Periodic pseudo-random generator.
    
    Randh generates a pseudo-random number between `min` and `max` 
    values at a frequency specified by `freq` parameter. Randh will 
    hold generated value until next generation.
    
    Parentclass: PyoObject

    Parameters:

    min : float or PyoObject, optional
        Minimum value for the random generation. Defaults to 0.
    max : float or PyoObject, optional
        Maximum value for the random generation. Defaults to 1.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    
    Methods:

    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:
    
    min : float or PyoObject. Minimum value.
    max : float or PyoObject. Maximum value.
    freq : float or PyoObject. Polling frequency.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> rnd = Randh(400, 600, 4)
    >>> a = Sine(rnd, mul=.5).out()
    
    """
    def __init__(self, min=0., max=1., freq=1., mul=1, add=0):
        PyoObject.__init__(self)
        self._min = min
        self._max = max
        self._freq = freq
        self._mul = mul
        self._add = add
        min, max, freq, mul, add, lmax = convertArgsToLists(min, max, freq, mul, add)
        self._base_objs = [Randh_base(wrap(min,i), wrap(max,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['min', 'max', 'freq', 'mul', 'add']
  
    def setMin(self, x):
        """
        Replace the `min` attribute.
        
        Parameters:

        x : float or PyoObject
            new `min` attribute.
        
        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.
        
        Parameters:

        x : float or PyoObject
            new `max` attribute.
        
        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            new `freq` attribute.
        
        """
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(1., 2., 'lin', 'max', self._max),
                          SLMap(0.1, 20., 'lin', 'freq', self._freq),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def min(self): return self._min
    @min.setter
    def min(self, x): self.setMin(x)
    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

class Choice(PyoObject):
    """
    Periodically choose a new value from a user list.
    
    Choice chooses a new value from a predefined list of floats `choice`
    at a frequency specified by `freq` parameter. Choice will 
    hold choosen value until next generation.
    
    Parentclass: PyoObject

    Parameters:

    choice : list of floats or list of lists of floats
        Possible values for the random generation.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    
    Methods:

    setChoice(x) : Replace the `choice` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:
    
    choice : list of floats or list of lists of floats. Possible choices.
    freq : float or PyoObject. Polling frequency.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> rnd = Choice([200,300,400,500,600], 4)
    >>> a = Sine(rnd, mul=.5).out()
    
    """
    def __init__(self, choice, freq=1., mul=1, add=0):
        PyoObject.__init__(self)
        self._choice = choice
        self._freq = freq
        self._mul = mul
        self._add = add
        freq, mul, add, lmax = convertArgsToLists(freq, mul, add)
        if type(choice[0]) != ListType:
            self._base_objs = [Choice_base(choice, wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        else:
            choicelen = len(choice)
            lmax = max(choicelen, lmax)
            self._base_objs = [Choice_base(wrap(choice,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
            
    def __dir__(self):
        return ['choice', 'freq', 'mul', 'add']

    def setChoice(self, x):
        """
        Replace the `choice` attribute.
        
        Parameters:

        x : list of floats or list of lists of floats
            new `choice` attribute.
        
        """
        self._choice = x
        if type(x[0]) != ListType:
            [obj.setChoice(self._choice) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setChoice(wrap(self._choice,i)) for i, obj in enumerate(self._base_objs)]
                
    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            new `freq` attribute.
        
        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.1, 20., 'lin', 'freq', self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def choice(self): return self._choice
    @choice.setter
    def choice(self, x): self.setChoice(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

class RandInt(PyoObject):
    """
    Periodic pseudo-random integer generator.
    
    RandInt generates a pseudo-random integer number between 0 and `max` 
    values at a frequency specified by `freq` parameter. RandInt will 
    hold generated value until the next generation.
    
    Parentclass: PyoObject

    Parameters:

    max : float or PyoObject, optional
        Maximum value for the random generation. Defaults to 100.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    
    Methods:

    setMax(x) : Replace the `max` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:
    
    max : float or PyoObject. Maximum value.
    freq : float or PyoObject. Polling frequency.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> rnd = RandInt(10, 5, 100, 500)
    >>> a = Sine(rnd, mul=.5).out()
    
    """
    def __init__(self, max=100, freq=1., mul=1, add=0):
        PyoObject.__init__(self)
        self._max = max
        self._freq = freq
        self._mul = mul
        self._add = add
        max, freq, mul, add, lmax = convertArgsToLists(max, freq, mul, add)
        self._base_objs = [RandInt_base(wrap(max,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['max', 'freq', 'mul', 'add']

    def setMax(self, x):
        """
        Replace the `max` attribute.
        
        Parameters:

        x : float or PyoObject
            new `max` attribute.
        
        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            new `freq` attribute.
        
        """
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(1., 2., 'lin', 'max', self._max),
                          SLMap(0.1, 20., 'lin', 'freq', self._freq),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

class RandDur(PyoObject):
    """
    Recursive time varying pseudo-random generator.

    RandDur generates a pseudo-random number between `min` and `max` 
    arguments and uses that number to set the delay time before the next 
    generation. RandDur will hold the generated value until next generation.

    Parentclass: PyoObject

    Parameters:

    min : float or PyoObject, optional
        Minimum value for the random generation. Defaults to 0.
    max : float or PyoObject, optional
        Maximum value for the random generation. Defaults to 1.

    Methods:

    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.

    Attributes:

    min : float or PyoObject. Minimum value.
    max : float or PyoObject. Maximum value.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> dur = RandDur(min=[.05,0.1], max=[.2,.25])
    >>> trig = Change(dur)
    >>> amp = TrigEnv(trig, table=HannTable(), dur=dur, mul=.2)
    >>> freq = TrigChoice(trig, choice=[100,150,200,250,300,350,400])
    >>> a = LFO(freq=freq, type=2, mul=amp).out()

    """
    def __init__(self, min=0., max=1., mul=1, add=0):
        PyoObject.__init__(self)
        self._min = min
        self._max = max
        self._mul = mul
        self._add = add
        min, max, mul, add, lmax = convertArgsToLists(min, max, mul, add)
        self._base_objs = [RandDur_base(wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['min', 'max', 'mul', 'add']

    def setMin(self, x):
        """
        Replace the `min` attribute.

        Parameters:

        x : float or PyoObject
            new `min` attribute.

        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        Parameters:

        x : float or PyoObject
            new `max` attribute.

        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(1., 2., 'lin', 'max', self._max),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def min(self): return self._min
    @min.setter
    def min(self, x): self.setMin(x)
    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class Xnoise(PyoObject):
    """
    X-class pseudo-random generator.

    Xnoise implements a few of the most common noise distributions.
    Each distribution generates values in the range 0 and 1.

    Parentclass: PyoObject
    
    Notes:
    
    Available distributions are:
        - uniform
        - linear minimum
        - linear maximum
        - triangular
        - exponential minimum
        - exponential maximum
        - double (bi)exponential
        - cauchy
        - weibull
        - gaussian
        - poisson
        - walker (drunk)
        - loopseg (drunk with looped segments)
        
    Depending on the distribution, `x1` and `x2` parameters are applied
    as follow (names as string, or associated number can be used as `dist`
    parameter):
        0 - uniform
            no parameter
        1 - linear_min 
            no parameter
        2 - linear_max
            no parameter
        3 - triangle
            no parameter
        4 - expon_min
            x1 : slope {0 = no slope -> 10 = sharp slope}
        5 - expon_max    
            x1 : slope {0 = no slope -> 10 = sharp slope}
        6 - biexpon
            x1 : bandwidth {0 = huge bandwidth -> 10 = narrow bandwidth}
        7 - cauchy
            x1 : bandwidth {0 = narrow bandwidth -> 10 = huge bandwidth}
        8 - weibull
            x1 : mean location {0 -> 1}
            x2 : shape {0.5 = linear min, 1.5 = expon min, 3.5 = gaussian}
        9 - gaussian
            x1 : mean location {0 -> 1}
            x2 : bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth}
        10 - poisson
            x1 : gravity center {0 = low values -> 10 = high values}
            x2 : compress/expand range {0.1 = full compress -> 4 full expand}
        11 - walker
            x1 : maximum value {0.1 -> 1}
            x2 - maximum step {0.1 -> 1}
        12 - loopseg 
            x1 : maximum value {0.1 -> 1}
            x2 - maximum step {0.1 -> 1}

    Parameters:

    dist : string or int, optional
        Distribution type. Defaults to 0.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    x1 : float or PyoObject, optional
        First parameter. Defaults to 0.5.
    x2 : float or PyoObject, optional
        Second parameter. Defaults to 0.5.

    Methods:

    setDist(x) : Replace the `dist` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setX1(x) : Replace the `x1` attribute.
    setX2(x) : Replace the `x2` attribute.

    Attributes:

    dist : string or int. Distribution type.
    freq : float or PyoObject. Polling frequency.
    x1 : float or PyoObject. First parameter.
    x2 : float or PyoObject. Second parameter.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Phasor(.1, 0, .5, .15)
    >>> a = Xnoise(dist=12, freq=8, x1=1, x2=lfo, mul=1000, add=500)
    >>> b = Sine(a, mul=.3).out()

    """
    def __init__(self, dist=0, freq=1., x1=0.5, x2=0.5, mul=1, add=0):
        PyoObject.__init__(self)
        self._dist = dist
        self._freq = freq
        self._x1 = x1
        self._x2 = x2
        self._mul = mul
        self._add = add
        dist, freq, x1, x2, mul, add, lmax = convertArgsToLists(dist, freq, x1, x2, mul, add)
        for i, t in enumerate(dist):
            if type(t) == StringType: dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [Xnoise_base(wrap(dist,i), wrap(freq,i), wrap(x1,i), wrap(x2,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['dist', 'freq', 'x1', 'x2', 'mul', 'add']

    def setDist(self, x):
        """
        Replace the `dist` attribute.

        Parameters:

        x : int
            new `dist` attribute.

        """
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) == StringType: x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        """
        Replace the `x1` attribute.

        Parameters:

        x : float or PyoObject
            new `x1` attribute.

        """
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        """
        Replace the `x2` attribute.

        Parameters:

        x : float or PyoObject
            new `x2` attribute.

        """
        self._x2= x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        Parameters:

        x : float or PyoObject
            new `freq` attribute.

        """
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def dist(self): return self._dist
    @dist.setter
    def dist(self, x): self.setDist(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)
    @property
    def x1(self): return self._x1
    @x1.setter
    def x1(self, x): self.setX1(x)
    @property
    def x2(self): return self._x2
    @x2.setter
    def x2(self, x): self.setX2(x)

class XnoiseMidi(PyoObject):
    """
    X-class midi notes pseudo-random generator.

    XnoiseMidi implements a few of the most common noise distributions.
    Each distribution generates integer values in the range defined with
    `mrange` parameter and output can be scaled on midi notes, hertz or 
    transposition factor. 

    Parentclass: PyoObject
    
    Notes:
    
    Available distributions are:
        - uniform
        - linear minimum
        - linear maximum
        - triangular
        - exponential minimum
        - exponential maximum
        - double (bi)exponential
        - cauchy
        - weibull
        - gaussian
        - poisson
        - walker (drunk)
        - loopseg (drunk with looped segments)

    Depending on the distribution, `x1` and `x2` parameters are applied
    as follow (names as string, or associated number can be used as `dist`
    parameter):
        0 - uniform
            no parameter
        1 - linear_min 
            no parameter
        2 - linear_max
            no parameter
        3 - triangle
            no parameter
        4 - expon_min
            x1 : slope {0 = no slope -> 10 = sharp slope}
        5 - expon_max    
            x1 : slope {0 = no slope -> 10 = sharp slope}
        6 - biexpon
            x1 : bandwidth {0 = huge bandwidth -> 10 = narrow bandwidth}
        7 - cauchy
            x1 : bandwidth {0 = narrow bandwidth -> 10 = huge bandwidth}
        8 - weibull
            x1 : mean location {0 -> 1}
            x2 : shape {0.5 = linear min, 1.5 = expon min, 3.5 = gaussian}
        9 - gaussian
            x1 : mean location {0 -> 1}
            x2 : bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth}
        10 - poisson
            x1 : gravity center {0 = low values -> 10 = high values}
            x2 : compress/expand range {0.1 = full compress -> 4 full expand}
        11 - walker
            x1 : maximum value {0.1 -> 1}
            x2 - maximum step {0.1 -> 1}
        12 - loopseg 
            x1 : maximum value {0.1 -> 1}
            x2 - maximum step {0.1 -> 1}

    Parameters:

    dist : string or int, optional
        Distribution type. Defaults to 0.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    x1 : float or PyoObject, optional
        First parameter. Defaults to 0.5.
    x2 : float or PyoObject, optional
        Second parameter. Defaults to 0.5.
    scale : int {0, 1, 2}, optional
        Output format. 0 = Midi, 1 = Hertz, 2 = transposition factor. 
        In the transposition mode, the central key (the key where there 
        is no transposition) is (`minrange` + `maxrange`) / 2. Defaults
        to 0.
    mrange : tuple of int, optional
        Minimum and maximum possible values, in Midi notes. Available
        only at initialization time. Defaults to (0, 127).

    Methods:

    setDist(x) : Replace the `dist` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setX1(x) : Replace the `x1` attribute.
    setX2(x) : Replace the `x2` attribute.
    setScale(x) : Replace the `scale` attribute.
    setRange(x, y) : Changes min and max range values and centralkey.

    Attributes:

    dist : string or int. Distribution type.
    freq : float or PyoObject. Polling frequency.
    x1 : float or PyoObject. First parameter.
    x2 : float or PyoObject. Second parameter.
    scale : int. Output format.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> l = Phasor(.1, 0, .5, 0)
    >>> a = XnoiseMidi('loopseg', freq=8, x1=1, x2=l, scale=1, mrange=(60,96))
    >>> b = Sine(a, mul=.3).out()

    """
    def __init__(self, dist=0, freq=1., x1=0.5, x2=0.5, scale=0, mrange=(0,127), mul=1, add=0):
        PyoObject.__init__(self)
        self._dist = dist
        self._freq = freq
        self._x1 = x1
        self._x2 = x2
        self._scale = scale
        self._mrange = mrange
        self._mul = mul
        self._add = add
        dist, freq, x1, x2, scale, mrange, mul, add, lmax = convertArgsToLists(dist, freq, x1, x2, scale, mrange, mul, add)
        for i, t in enumerate(dist):
            if type(t) == StringType: dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [XnoiseMidi_base(wrap(dist,i), wrap(freq,i), wrap(x1,i), wrap(x2,i), wrap(scale,i), wrap(mrange,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['dist', 'freq', 'x1', 'x2', 'scale', 'mul', 'add']

    def setDist(self, x):
        """
        Replace the `dist` attribute.

        Parameters:

        x : string or int
            new `dist` attribute.

        """
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) == StringType: x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        """
        Replace the `scale` attribute.
        
        Possible values are: 
            0 -> Midi notes
            1 -> Hertz
            2 -> transposition factor 
                 (centralkey is (`minrange` + `maxrange`) / 2

        Parameters:

        x : int {0, 1, 2}
            new `scale` attribute.

        """
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setRange(self, mini, maxi):
        """
        Replace the `mrange` attribute.

        Parameters:

        mini : int
            minimum output midi range.
        maxi : int
            maximum output midi range.

        """
        self._mrange = (mini, maxi)
        mini, maxi, lmax = convertArgsToLists(mini, maxi)
        [obj.setRange(wrap(mini,i), wrap(maxi,i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        """
        Replace the `x1` attribute.

        Parameters:

        x : float or PyoObject
            new `x1` attribute.

        """
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        """
        Replace the `x2` attribute.

        Parameters:

        x : float or PyoObject
            new `x2` attribute.

        """
        self._x2= x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        Parameters:

        x : float or PyoObject
            new `freq` attribute.

        """
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def dist(self): return self._dist
    @dist.setter
    def dist(self, x): self.setDist(x)
    @property
    def scale(self): return self._scale
    @scale.setter
    def scale(self, x): self.setScale(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)
    @property
    def x1(self): return self._x1
    @x1.setter
    def x1(self, x): self.setX1(x)
    @property
    def x2(self): return self._x2
    @x2.setter
    def x2(self, x): self.setX2(x)

class XnoiseDur(PyoObject):
    """
    Recursive time varying X-class pseudo-random generator.

    Xnoise implements a few of the most common noise distributions.
    Each distribution generates values in the range 0 to 1, which are
    then rescaled between `min` and `max` arguments. The object uses 
    the generated value to set the delay time before the next generation. 
    XnoiseDur will hold the value until next generation.

    Parentclass: PyoObject

    Notes:

    Available distributions are:
        - uniform
        - linear minimum
        - linear maximum
        - triangular
        - exponential minimum
        - exponential maximum
        - double (bi)exponential
        - cauchy
        - weibull
        - gaussian
        - poisson
        - walker (drunk)
        - loopseg (drunk with looped segments)

    Depending on the distribution, `x1` and `x2` parameters are applied
    as follow (names as string, or associated number can be used as `dist`
    parameter):
        0 - uniform
            no parameter
        1 - linear_min 
            no parameter
        2 - linear_max
            no parameter
        3 - triangle
            no parameter
        4 - expon_min
            x1 : slope {0 = no slope -> 10 = sharp slope}
        5 - expon_max    
            x1 : slope {0 = no slope -> 10 = sharp slope}
        6 - biexpon
            x1 : bandwidth {0 = huge bandwidth -> 10 = narrow bandwidth}
        7 - cauchy
            x1 : bandwidth {0 = narrow bandwidth -> 10 = huge bandwidth}
        8 - weibull
            x1 : mean location {0 -> 1}
            x2 : shape {0.5 = linear min, 1.5 = expon min, 3.5 = gaussian}
        9 - gaussian
            x1 : mean location {0 -> 1}
            x2 : bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth}
        10 - poisson
            x1 : gravity center {0 = low values -> 10 = high values}
            x2 : compress/expand range {0.1 = full compress -> 4 full expand}
        11 - walker
            x1 : maximum value {0.1 -> 1}
            x2 - maximum step {0.1 -> 1}
        12 - loopseg 
            x1 : maximum value {0.1 -> 1}
            x2 - maximum step {0.1 -> 1}

    Parameters:

    dist : string or int, optional
        Distribution type. Can be the name of the distribution as a string
        or its associated number. Defaults to 0.
    min : float or PyoObject, optional
        Minimum value for the random generation. Defaults to 0.
    max : float or PyoObject, optional
        Maximum value for the random generation. Defaults to 1.
    x1 : float or PyoObject, optional
        First parameter. Defaults to 0.5.
    x2 : float or PyoObject, optional
        Second parameter. Defaults to 0.5.

    Methods:

    setDist(x) : Replace the `dist` attribute.
    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.
    setX1(x) : Replace the `x1` attribute.
    setX2(x) : Replace the `x2` attribute.

    Attributes:

    dist : string or int. Distribution type.
    min : float or PyoObject. Minimum value.
    max : float or PyoObject. Maximum value.
    x1 : float or PyoObject. First parameter.
    x2 : float or PyoObject. Second parameter.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> dur = XnoiseDur(dist="expon_min", min=[.05,0.1], max=[.2,.25], x1=3)
    >>> trig = Change(dur)
    >>> amp = TrigEnv(trig, table=HannTable(), dur=dur, mul=.2)
    >>> freq = TrigChoice(trig, choice=[100,150,200,250,300,350,400])
    >>> a = LFO(freq=freq, type=2, mul=amp).out()

    """
    def __init__(self, dist=0, min=0., max=1., x1=0.5, x2=0.5, mul=1, add=0):
        PyoObject.__init__(self)
        self._dist = dist
        self._min = min
        self._max = max
        self._x1 = x1
        self._x2 = x2
        self._mul = mul
        self._add = add
        dist, min, max, x1, x2, mul, add, lmax = convertArgsToLists(dist, min, max, x1, x2, mul, add)
        for i, t in enumerate(dist):
            if type(t) == StringType: dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [XnoiseDur_base(wrap(dist,i), wrap(min,i), wrap(max,i), wrap(x1,i), wrap(x2,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['dist', 'min', 'max', 'x1', 'x2', 'mul', 'add']

    def setDist(self, x):
        """
        Replace the `dist` attribute.

        Parameters:

        x : int
            new `dist` attribute.

        """
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) == StringType: x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMin(self, x):
        """
        Replace the `min` attribute.

        Parameters:

        x : float or PyoObject
            new `min` attribute.

        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        Parameters:

        x : float or PyoObject
            new `max` attribute.

        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        """
        Replace the `x1` attribute.

        Parameters:

        x : float or PyoObject
            new `x1` attribute.

        """
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        """
        Replace the `x2` attribute.

        Parameters:

        x : float or PyoObject
            new `x2` attribute.

        """
        self._x2= x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def dist(self): return self._dist
    @dist.setter
    def dist(self, x): self.setDist(x)
    @property
    def min(self): return self._min
    @min.setter
    def min(self, x): self.setMin(x)
    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)
    @property
    def x1(self): return self._x1
    @x1.setter
    def x1(self, x): self.setX1(x)
    @property
    def x2(self): return self._x2
    @x2.setter
    def x2(self, x): self.setX2(x)
