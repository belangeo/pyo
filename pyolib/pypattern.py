import threading, time
from _core import *

class PyPattern(threading.Thread):
    """
    Periodically call a Python function.
        
    This class is a pure Python implementation of the Pattern object.
    It uses the threading module to run his clock in another thread.
    
    Parameters:

    function : Python function
    time : float or PyoObject, optional
        Time, in seconds, between each call. Default to 1.
        
    Methods:

    setTime(x) : Replace the `time` attribute.

    Attributes:
    
    time : Time, in seconds, between each call.
    
    Notes:

    The out() method is bypassed. Pattern doesn't return signal.
    
    Pattern has no `mul` and `add` attributes.
    
    """
    def __init__(self, function, time=1):
        threading.Thread.__init__(self)
        self._terminated = False
        self._function = function
        self._time = time
        
    def run(self):
        while not self._terminated:
            self._function()
            time.sleep(self._time)
            
    def play(self):
        self._terminated = False
        try:
            self.start()
        except:
            pass
 
    def out(self, chnl=0, inc=1):
        return self
        
    def stop(self):
        self._terminated = True        
    
    def setTime(self, x):
        self._time = x

    #def demo():
    #    execfile("pyodemos/PyPattern_demo.py")
    #demo = Call_example(demo)

    def args():
        return('PyPattern(function, time=1)')
    args = Print_args(args)
        
    @property
    def time(self):
        """float or PyoObject. Time, in seconds, between each call.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)

