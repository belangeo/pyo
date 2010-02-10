from _core import *

######################################################################
### Controls
######################################################################                                       
class Fader(PyoObject):
    """
    Fadein - fadeout envelope generator.
    
    Generate an amplitude envelope between 0 and 1 with control on fade 
    times and total duration of the envelope.
    
    The play() method starts the envelope and is not called at the 
    object creation time.
    
    Parent class: PyoObject

    Parameters:

    fadein : float, optional
        Rising time of the envelope in seconds. Defaults to 0.01.
    fadeout : float, optional
        Falling time of the envelope in seconds. Defaults to 0.1.
    dur : float, optional
        Total duration of the envelope. Defaults to 0, which means wait 
        for the stop() method to start the fadeout.
        
    Methods:

    play() : Start processing without sending samples to the output. 
        Triggers the envelope.
    stop() : Stop processing. Triggers the envelope's fadeout 
        if `dur` is set to 0.
    setFadein(x) : Replace the `fadein` attribute.
    setFadeout(x) : Replace the `fadeout` attribute.
    setDur(x) : Replace the `dur` attribute.

    Attributes:
    
    fadein : float. Rising time of the envelope in seconds.
    fadeout : float. Falling time of the envelope in seconds.
    dur : float. Total duration of the envelope.
    
    Notes:

    The out() method is bypassed. Fader's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> f = Fader(fadein=1, fadeout=2, dur=5, mul=.5)
    >>> a = Noise(mul=f).out()
    >>> f.play()    
    
    """
    def __init__(self, fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0):
        self._fadein = fadein
        self._fadeout = fadeout
        self._dur = dur
        self._mul = mul
        self._add = add
        fadein, fadeout, dur, mul, add, lmax = convertArgsToLists(fadein, fadeout, dur, mul, add)
        self._base_objs = [Fader_base(wrap(fadein,i), wrap(fadeout,i), wrap(dur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1):
        """
        Bypassed. Can't be sent to audio outs.
        """
        pass

    def setFadein(self, x):
        """
        Replace the `fadein` attribute.
        
        Parameters:

        x : float
            new `fadein` attribute.
        
        """
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        """
        Replace the `fadeout` attribute.
        
        Parameters:

        x : float
            new `fadeout` attribute.
        
        """
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.
        
        Parameters:

        x : float
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control on Fader object."

    def demo():
        execfile(DEMOS_PATH + "/Fader_demo.py")
    demo = Call_example(demo)

    def args():
        return("Fader(fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0)")
    args = Print_args(args)

    @property
    def fadein(self):
        """float. Rising time of the envelope in seconds.""" 
        return self._fadein
    @fadein.setter
    def fadein(self, x): self.setFadein(x)

    @property
    def fadeout(self):
        """float. Falling time of the envelope in seconds.""" 
        return self._fadeout
    @fadeout.setter
    def fadeout(self, x): self.setFadeout(x)

    @property
    def dur(self):
        """float. Total duration of the envelope.""" 
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

