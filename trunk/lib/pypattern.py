import threading, time
from _core import *

class PyPattern(threading.Thread):
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
                    
    def stop(self):
        self._terminated = True        
    
    def setTime(self, x):
        self._time = x

    #def demo():
    #    execfile("demos/PyPattern_demo.py")
    #demo = Call_example(demo)

    def args():
        print('PyPattern(function, time=1)')
    args = Print_args(args)
        
    @property
    def time(self):
        """float or PyoObject. Time, in seconds, between each call.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)

