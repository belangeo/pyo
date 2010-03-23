from _core import *
from types import StringType

class Randi(PyoObject):
    """
    Periodic pseudo-random generator with interpolation.
    
    Randi generates a pseudo-random number between `min` and `max` 
    values at a frequency specified by `freq` parameter. Randi will 
    produce straight-line interpolation between current number and the next.
    
    Parent class: PyoObject

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

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(1., 2., 'lin', 'max', self._max),
                          SLMap(0.1, 20., 'lin', 'freq', self._freq),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    
    Randi generates a pseudo-random number between `min` and `max` 
    values at a frequency specified by `freq` parameter. Randi will 
    hold generated value until next generation.
    
    Parent class: PyoObject

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

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(1., 2., 'lin', 'max', self._max),
                          SLMap(0.1, 20., 'lin', 'freq', self._freq),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    
    Parent class: PyoObject

    Parameters:

    choice : list of floats
        Possible values for the random generation.
    freq : float or PyoObject, optional
        Polling frequency. Defaults to 1.
    
    Methods:

    setChoice(x) : Replace the `choice` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:
    
    choice : list of floats. Possible choices.
    freq : float or PyoObject. Polling frequency.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> rnd = Choice([200,300,400,500,600], 4)
    >>> a = Sine(rnd, mul=.5).out()
    
    """
    def __init__(self, choice, freq=1., mul=1, add=0):
        self._choice = choice
        self._freq = freq
        self._mul = mul
        self._add = add
        freq, mul, add, lmax = convertArgsToLists(freq, mul, add)
        self._base_objs = [Choice_base(choice, wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['choice', 'freq', 'mul', 'add']

    def setChoice(self, x):
        """
        Replace the `choice` attribute.
        
        Parameters:

        x : list of floats
            new `choice` attribute.
        
        """
        self._choice = x
        [obj.setChoice(self._choice) for i, obj in enumerate(self._base_objs)]

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

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0.1, 20., 'lin', 'freq', self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    
    Parent class: PyoObject

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

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(1., 2., 'lin', 'max', self._max),
                          SLMap(0.1, 20., 'lin', 'freq', self._freq),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

class Xnoise(PyoObject):
    """
    X-class pseudo-random generator.

    Xnoise implements a few of the most common noise distributions.
    Each distribution generates values in the range 0 and 1.
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
           
    Parent class: PyoObject

    Parameters:

    dist : string of int, optional
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

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

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
