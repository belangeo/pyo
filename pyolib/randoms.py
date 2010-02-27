from _core import *

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
        if map_list == None:
            map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                        SLMap(1., 2., 'lin', 'max', self._max),
                        SLMap(0.1, 20., 'lin', 'freq', self._freq),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/Randi_demo.py")
    #demo = Call_example(demo)

    def args():
        return('Randi(min=0., max=1., freq=1., mul=1, add=0)')
    args = Print_args(args)

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
