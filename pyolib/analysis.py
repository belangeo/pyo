"""
Tools to analyze audio signals.

"""
from _core import *

class Follower(PyoObject):
    """
    Envelope follower.
    
    Output signal is the continuous mean amplitude of an input signal.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Cutoff frequency of the filter in hertz. Default to 10.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Cutoff frequency of the filter.

    Notes:

    The out() method is bypassed. Follower's signal can not be sent to 
    audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True).out()
    >>> fol = Follower(sf, freq=10)
    >>> n = Noise(mul=fol).out()

    """
    def __init__(self, input, freq=10, mul=1, add=0):
        self._input = input
        self._freq = freq
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [Follower_base(wrap(in_fader,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            New `freq` attribute.

        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1):
        pass

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(1., 500., 'log', 'freq', self._freq)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Follower_demo.py")
    demo = Call_example(demo)

    def args():
        return("Follower(input, freq=10, mul=1, add=0)")
    args = Print_args(args)
      
    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the filter.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

