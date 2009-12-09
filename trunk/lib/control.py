from _core import *

######################################################################
### Controls
######################################################################                                       
class Fader(PyoObject):
    """
    Generate an amplitude envelope between 0 and 1 with control on fade times and duration.
    
    The play() method starts the envelope and is not called at the object creation.
    
    **Attributes**

    fadein : float or PyoObject, optional
        Rising time of the envelope in seconds. Default to 0.01.
    fadeout : float or PyoObject, optional
        Falling time of the envelope in seconds. Default to 0.1.
    dur : float or PyoObject, optional
        Total duration of the envelope. Default to 0, which mean wait for the stop() 
        method to start the fadeout.
        
    **Methods**

    play() : Start processing without sending samples to output and trigger the envelope.
    stop() : Stop processing and trigger the envelope fadeout if `dur` is set to 0.
    setFadein(x) : Replace the `fadein` attribute.
    setFadeout(x) : Replace the `fadeout` attribute.
    setDur(x) : Replace the `dur` attribute.

    **Notes**

    Methods out() is bypassed. Fader signal can't be sent to audio outs.
    
    """
    def __init__(self, fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0):
        self._fadein = fadein
        self._fadeout = fadeout
        self._dur = dur
        self._mul = mul
        self._add = add
        fadein, fadeout, dur, mul, add, lmax = convertArgsToLists(fadein, fadeout, dur, mul, add)
        self._base_objs = [Fader_base(wrap(fadein,i), wrap(fadeout,i), wrap(dur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0):
        """Bypassed. Can't be sent to audio outs."""
        pass

    def setFadein(self, x):
        """Replace the `fadein` attribute.
        
        **Parameters**

        x : float or PyoObject
            new `fadein` attribute.
        
        """
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        """Replace the `fadeout` attribute.
        
        **Parameters**

        x : float or PyoObject
            new `fadeout` attribute.
        
        """
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """Replace the `dur` attribute.
        
        **Parameters**

        x : float or PyoObject
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def fadein(self): return self._fadein
    @property
    def fadeout(self): return self._fadeout
    @property
    def dur(self): return self._dur
    @fadein.setter
    def fadein(self, x): self.setFadein(x)
    @fadeout.setter
    def fadeout(self, x): self.setFadeout(x)
    @dur.setter
    def dur(self, x): self.setDur(x)

class Port(PyoObject):
    """
    Perform exponential portamento on an audio signal with different rising and falling times.
    
    **Parameters**

    input : PyoObject
        Input signal to filter.
    risetime : float or PyoObject, optional
        Time to reach upward value in seconds. Default to 0.05.
    falltime : float or PyoObject, optional
        Time to reach downward value in seconds. Default to 0.05.
        
    **Methods**

    setInput(x, fadetime) : Replace the `input` attribute.
    setRiseTime(x) : Replace the `risetime` attribute.
    setFallTime(x) : Replace the `falltime` attribute.
    
    """
    def __init__(self, input, risetime=0.05, falltime=0.05, mul=1, add=0):
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, mul, add)
        self._base_objs = [Port_base(wrap(in_fader,i), wrap(risetime,i), wrap(falltime,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        **Parameters**

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setRiseTime(self, x):
        """
        Replace the `risetime` attribute.
        
        **Parameters**

        x : float or PyoObject
            New `risetime` attribute.

        """
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        """
        Replace the `falltime` attribute.
        
        **Parameters**

        x : float or PyoObject
            New `falltime` attribute.

        """
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def risetime(self): return self._risetime
    @risetime.setter
    def risetime(self, x): self.setRiseTime(x)
    @property
    def falltime(self): return self._falltime
    @falltime.setter
    def falltime(self, x): self.setFallTime(x)

class Metro(PyoObject):
    """
    Generate isochronous trigger signals.
    
    A trigger is an audio signal with a value of 1 surounded by 0s.
    
    **Parameters**

    time : float or PyoObject, optional
        Time, in seconds, between each trigger. Default to 1.
    poly : int, optional
        Metronome polyphony. Denotes how many independent streams are generated by
        the metronome, allowing overlaping processes. Default to 1.
        
    **Methods**

    setTime(x) : Replace the `time` attribute.

    **Notes**

    Methods out() is bypassed. Metro signal can't be sent to audio outs.
    
    Metro has no `mul` and `add` attributes.
    
    """
    def __init__(self, time=1, poly=1):
        self._time = time
        time, lmax = convertArgsToLists(time)
        self._base_objs = [Metro_base(wrap(time,i)*poly, (float(j)/poly)) for i in range(lmax) for j in range(poly)]

    def setTime(self, x):
        """Replace the `time` attribute.
        
        **Parameters**
        
        x : float or PyoObject
            New `time` attribute.
        
        """
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, x=0):
        pass
        
    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass
         
    @property
    def time(self): return self._time
    @time.setter
    def time(self, x): self.setTime(x)
