from _core import *

######################################################################
### Sources
######################################################################                                       
class Sine(PyoObject):
    """
    A simple sine wave oscillator.
    
    Parent class: PyoObject
    
    Parameters:
    
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1000.
    phase : float or PyoObject, optional
        Phase of sampling, expressed as a fraction of a cycle (0 to 1). 
        Defaults to 0.
        
    Methods:
    
    setFreq(x) : Replace the `freq` attribute.
    setPhase(x) : Replace the `phase` attribute.
    
    Attributes:
    
    freq : float or PyoObject, Frequency in cycles per second.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    
    See also: Osc, Phasor
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sine = Sine(freq=500).out()
    
    """
    def __init__(self, freq=1000, phase=0, mul=1, add=0):
        self._freq = freq
        self._phase = phase
        self._mul = mul
        self._add = add
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Sine_base(wrap(freq,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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
        
    def setPhase(self, x):
        """
        Replace the `phase` attribute.
        
        Parameters:

        x : float or PyoObject
            new `phase` attribute.
        
        """
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapFreq(self._freq),
                        SLMapPhase(self._phase),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Sine_demo.py")
    demo = Call_example(demo)

    def args():
        return("Sine(freq=1000, phase=0, mul=1, add=0)")
    args = Print_args(args)
        
    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling.""" 
        return self._phase
    @phase.setter
    def phase(self, x): self.setPhase(x)

class Phasor(PyoObject):
    """
    A simple phase incrementor. 
    
    Output is a periodic ramp from 0 to 1.
 
    Parent class: PyoObject
   
    Parameters:
    
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 100.
    phase : float or PyoObject, optional
        Phase of sampling, expressed as a fraction of a cycle (0 to 1). 
        Defaults to 0.
        
    Methods:
    
    setFreq(x) : Replace the `freq` attribute.
    setPhase(x) : Replace the `phase` attribute.
 
    Attributes:
    
    freq : float or PyoObject, Frequency in cycles per second.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    
    See also: Osc, Sine
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> f = Phasor(freq=1, mul=1000, add=500)
    >>> sine = Sine(freq=f).out()   
    
    """
    def __init__(self, freq=100, phase=0, mul=1, add=0):
        self._freq = freq
        self._phase = phase
        self._mul = mul
        self._add = add
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Phasor_base(wrap(freq,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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
        
    def setPhase(self, x):
        """
        Replace the `phase` attribute.
        
        Parameters:

        x : float or PyoObject
            new `phase` attribute.
        
        """
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapFreq(self._freq),
                        SLMapPhase(self._phase),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/Phasor_demo.py")
    #demo = Call_example(demo)

    def args():
        return("Phasor(freq=100, phase=0, mul=1, add=0)")
    args = Print_args(args)
        
    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling.""" 
        return self._phase
    @phase.setter
    def phase(self, x): self.setPhase(x)

class Input(PyoObject):
    """
    Read from a numbered channel in an external audio signal or stream.

    Parent class: PyoObject

    Parameters:
    
    chnl : int, optional
        Input channel to read from. Defaults to 0.

    Notes:
    
    Requires that the Server's duplex mode is set to 1. 
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input(chnl=0)
    >>> b = Delay(a, delay=.25, feedback=.5, mul=.5).out()   
    
    """
    def __init__(self, chnl=0, mul=1, add=0):                
        self._chnl = chnl
        self._mul = mul
        self._add = add
        chnl, mul, add, lmax = convertArgsToLists(chnl, mul, add)
        self._base_objs = [Input_base(wrap(chnl,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Input_demo.py")
    demo = Call_example(demo)

    def args():
        return("Input(chnl=0, mul=1, add=0)")
    args = Print_args(args)

class Noise(PyoObject):
    """
    A white noise generator.
        
    Parent class: PyoObject
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise()
    >>> b = Biquad(a, freq=1000, q=5, type=0).out()    
        
    """
    def __init__(self, mul=1, add=0):                
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [Noise_base(wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Noise_demo.py")
    demo = Call_example(demo)

    def args():
        return("Noise(mul=1, add=0)")
    args = Print_args(args)
