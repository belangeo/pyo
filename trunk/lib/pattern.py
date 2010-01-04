from _core import *

class Pattern(PyoObject):
    """
    Periodically call a Python function.
        
    **Parameters**

    function : Python function
    time : float or PyoObject, optional
        Time, in seconds, between each call. Default to 1.
        
    **Methods**

    setTime(x) : Replace the `time` attribute.

    **Notes**

    The out() method is bypassed. Pattern doesn't return signal.
    
    Pattern has no `mul` and `add` attributes.
    
    """
    def __init__(self, function, time=1):
        self._function = function
        self._time = time
        function, time, lmax = convertArgsToLists(function, time)
        self._base_objs = [Pattern_base(wrap(function,i), wrap(time,i)) for i in range(lmax)]

    def setTime(self, x):
        """Replace the `time` attribute.
        
        **Parameters**
        
        x : float or PyoObject
            New `time` attribute.
        
        """
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, x=0, inc=1):
        pass
        
    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    #def demo():
    #    execfile("demos/Pattern_demo.py")
    #demo = Call_example(demo)

    def args():
        print('Pattern(function, time=1)')
    args = Print_args(args)
         
    @property
    def time(self): return self._time
    @time.setter
    def time(self, x): self.setTime(x)
