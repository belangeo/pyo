from _core import *

######################################################################
### Controls
######################################################################                                       
class Fader(PyoObject):
    """
    Generate an amplitude envelope between 0 and 1 with control on fade times and 
    total duration of the envelope.
    
    The play() method starts the envelope and is not called at the object creation time.
    
    Parent class: PyoObject

    Parameters:

    fadein : float or PyoObject, optional
        Rising time of the envelope in seconds. Defaults to 0.01.
    fadeout : float or PyoObject, optional
        Falling time of the envelope in seconds. Defaults to 0.1.
    dur : float or PyoObject, optional
        Total duration of the envelope. Defaults to 0, which means wait for the stop() 
        method to start the fadeout.
        
    Methods:

    play() : Start processing without sending samples to the output. Triggers the envelope.
    stop() : Stop processing. Triggers the envelope's fadeout if `dur` is set to 0.
    setFadein(x) : Replace the `fadein` attribute.
    setFadeout(x) : Replace the `fadeout` attribute.
    setDur(x) : Replace the `dur` attribute.

    Attributes:
    
    fadein : float or PyoObject. Rising time of the envelope in seconds.
    fadeout : float or PyoObject. Falling time of the envelope in seconds.
    dur : float or PyoObject. Total duration of the envelope.
    
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

        x : float or PyoObject
            new `fadein` attribute.
        
        """
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        """
        Replace the `fadeout` attribute.
        
        Parameters:

        x : float or PyoObject
            new `fadeout` attribute.
        
        """
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.
        
        Parameters:

        x : float or PyoObject
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def demo():
        execfile("demos/Fader_demo.py")
    demo = Call_example(demo)

    def args():
        return("Fader(fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0)")
    args = Print_args(args)

    @property
    def fadein(self):
        """float or PyoObject. Rising time of the envelope in seconds.""" 
        return self._fadein
    @fadein.setter
    def fadein(self, x): self.setFadein(x)

    @property
    def fadeout(self):
        """float or PyoObject. Falling time of the envelope in seconds.""" 
        return self._fadeout
    @fadeout.setter
    def fadeout(self, x): self.setFadeout(x)

    @property
    def dur(self):
        """float or PyoObject. Total duration of the envelope.""" 
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

class Port(PyoObject):
    """
    Perform an exponential portamento on an audio signal with different rising and falling times.
    
    Parent class: PyoObject
    
    Parameters:

    input : PyoObject
        Input signal to filter.
    risetime : float or PyoObject, optional
        Time to reach upward value in seconds. Defaults to 0.05.
    falltime : float or PyoObject, optional
        Time to reach downward value in seconds. Defaults to 0.05.
    init : float, optional
        Initial state of the internal memory. Available at intialization time only.
        Defaults to 0.
        
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setRiseTime(x) : Replace the `risetime` attribute.
    setFallTime(x) : Replace the `falltime` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    risetime : float or PyoObject. Time to reach upward value in seconds.
    falltime : float or PyoObject. Time to reach downward value in seconds.
     
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> x = Sig(value=500)
    >>> p = Port(x, risetime=.1, falltime=1)
    >>> a = Sine(freq=p, mul=.5).out()
    >>> x.value = 1000
    >>> x.value = 600
    
    """
    def __init__(self, input, risetime=0.05, falltime=0.05, init=0, mul=1, add=0):
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, init, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, init, mul, add)
        self._base_objs = [Port_base(wrap(in_fader,i), wrap(risetime,i), wrap(falltime,i), wrap(init,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setRiseTime(self, x):
        """
        Replace the `risetime` attribute.
        
        Parameters:

        x : float or PyoObject
            New `risetime` attribute.

        """
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        """
        Replace the `falltime` attribute.
        
        Parameters:

        x : float or PyoObject
            New `falltime` attribute.

        """
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def demo():
        execfile("demos/Port_demo.py")
    demo = Call_example(demo)

    def args():
        return("Port(input, risetime=0.05, falltime=0.05, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def risetime(self):
        """float or PyoObject. Time to reach upward value in seconds.""" 
        return self._risetime
    @risetime.setter
    def risetime(self, x): self.setRiseTime(x)

    @property
    def falltime(self):
        """float or PyoObject. Time to reach downward value in seconds."""
        return self._falltime
    @falltime.setter
    def falltime(self, x): self.setFallTime(x)

class Metro(PyoObject):
    """
    Generate isochronous trigger signals.
    
    A trigger is an audio signal with a value of 1 surrounded by 0s.

    The play() method starts the metro and is not called at the object creation time.
    
    Parent class: PyoObject
    
    Parameters:

    time : float or PyoObject, optional
        Time between each trigger in seconds. Defaults to 1.
    poly : int, optional
        Metronome polyphony. Denotes how many independent streams are generated by
        the metronome, allowing overlapping processes. Available only at initialization.
        Defaults to 1.
        
    Methods:

    setTime(x) : Replace the `time` attribute.

    Attributes:
    
    time : float or PyoObject. Time between each trigger in seconds.
    
    Notes:

    The out() method is bypassed. Metro's signal can not be sent to audio outs.
    
    Metro has no `mul` and `add` attributes.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Metro(time=.125).play()
    >>> t = TrigRand(m, min=400, max=1000)
    >>> a = Sine(freq=t, mul=.5).out()
    
    """
    def __init__(self, time=1, poly=1):
        self._time = time
        self._poly = poly
        time, lmax = convertArgsToLists(time)
        self._base_objs = [Metro_base(wrap(time,i)*poly, (float(j)/poly)) for i in range(lmax) for j in range(poly)]

    def setTime(self, x):
        """
        Replace the `time` attribute.
        
        Parameters:
        
        x : float or PyoObject
            New `time` attribute.
        
        """
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)*self._poly) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1):
        pass
        
    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def demo():
        execfile("demos/Metro_demo.py")
    demo = Call_example(demo)

    def args():
        return("Metro(time=1, poly=1)")
    args = Print_args(args)
         
    @property
    def time(self):
        """float or PyoObject. Time between each trigger in seconds.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)

class Follower(PyoObject):
    """
    Envelope follower. 
 
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

    The out() method is bypassed. Follower's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer("demos/transparent.aif", loop=True).out()
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

    def demo():
        execfile("demos/Follower_demo.py")
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

