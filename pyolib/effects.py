from _core import *

######################################################################
### Effects
######################################################################                                       
class Biquad(PyoObject):
    """
    A sweepable general purpose biquadratic digital filter. 
    
    Parent class : PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Cutoff or center frequency of the filter. Defaults to 1000.
    q : float or PyoObject, optional
        Q of the filter, defined (for bandpass filters) as bandwidth/cutoff. 
        Should be between 1 and 500. Defaults to 1.
    type : int, optional
        Filter type. Four possible values :
            0 = lowpass (default)
            1 = highpass
            2 = bandpass
            3 = bandstop

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setQ(x) : Replace the `q` attribute.
    setType(x) : Replace the `type` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Cutoff or center frequency of the filter.
    q : float or PyoObject. Q of the filter.
    type : int. Filter type.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.5)
    >>> lfo = Sine(freq=.25, mul=1000, add=1000)
    >>> f = Biquad(a, freq=lfo, q=5, type=2).out()

    """
    def __init__(self, input, freq=1000, q=1, type=0, mul=1, add=0):
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, type, mul, add)
        self._base_objs = [Biquad_base(wrap(in_fader,i), wrap(freq,i), wrap(q,i), wrap(type,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.
        
        Parameters:

        x : float or PyoObject
            New `q` attribute.

        """
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.
        
        Parameters:

        x : int
            New `type` attribute. 
            0 = lowpass, 1 = highpass, 2 = bandpass, 3 = bandstop.

        """
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapFreq(self._freq),
                        SLMapQ(self._q),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Biquad_demo.py")
    demo = Call_example(demo)

    def args():
        return("Biquad(input, freq=1000, q=1, type=0, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff or center frequency of the filter.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter.""" 
        return self._q
    @q.setter
    def q(self, x): self.setQ(x)

    @property
    def type(self):
        """int. Filter type.""" 
        return self._type
    @type.setter
    def type(self, x): self.setType(x)

class Tone(PyoObject):
    """
    A first-order recursive low-pass filter with variable frequency response.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Cutoff frequency of the filter in hertz. Default to 1000.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Cutoff frequency of the filter.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.5)
    >>> f = Tone(n, 500).out()

    """
    def __init__(self, input, freq=1000, mul=1, add=0):
        self._input = input
        self._freq = freq
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [Tone_base(wrap(in_fader,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        
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

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapFreq(self._freq),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/Tone_demo.py")
    #demo = Call_example(demo)

    def args():
        return("Tone(input, freq=1000, mul=1, add=0)")
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

class Port(PyoObject):
    """
    Perform an exponential portamento on an audio signal with 
    different rising and falling times.
    
    Parent class: PyoObject
    
    Parameters:

    input : PyoObject
        Input signal to filter.
    risetime : float or PyoObject, optional
        Time to reach upward value in seconds. Defaults to 0.05.
    falltime : float or PyoObject, optional
        Time to reach downward value in seconds. Defaults to 0.05.
    init : float, optional
        Initial state of the internal memory. Available at intialization 
        time only. Defaults to 0.
        
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

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0.001, 10., 'lin', 'risetime', self._risetime),
                        SLMap(0.001, 10., 'lin', 'falltime', self._falltime)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Port_demo.py")
    demo = Call_example(demo)

    def args():
        return("Port(input, risetime=0.05, falltime=0.05, init=0, mul=1, add=0)")
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

class DCBlock(PyoObject):
    """
    Implements the DC blocking filter.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.05)
    >>> w = Delay(n, delay=0.01, feedback=.995, mul=.5)
    >>> f = DCBlock(w).out()

    """
    def __init__(self, input, mul=1, add=0):
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [DCBlock_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        
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

    def ctrl(self, map_list=None, title=None):
        print "There is no control for DCBlock object."
        
    #def demo():
    #    execfile(DEMOS_PATH + "/DCBlock_demo.py")
    #demo = Call_example(demo)

    def args():
        return("DCBlock(input, mul=1, add=0)")
    args = Print_args(args)
      
    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Disto(PyoObject):
    """
    Arctan distortion.
    
    Apply an arctan distortion with controllable drive to the input signal. 
    
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to process.
    drive : float or PyoObject, optional
        Amount of distortion applied to the signal, between 0 and 1. 
        Defaults to 0.75.
    slope : float or PyoObject, optional
        Slope of the lowpass filter applied after distortion, 
        between 0 and 1. Defaults to 0.5.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setDrive(x) : Replace the `drive` attribute.
    setSlope(x) : Replace the `slope` attribute.

    Attributes:
    
    input : PyoObject. Input signal to filter.
    drive : float or PyoObject. Amount of distortion.
    slope : float or PyoObject. Slope of the lowpass filter.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    >>> lfo = Sine(freq=.25, mul=.5, add=.5)
    >>> d = Disto(a, drive=lfo, slope=.8, mul=.5).out()

    """
    def __init__(self, input, drive=.75, slope=.5, mul=1, add=0):
        self._input = input
        self._drive = drive
        self._slope = slope
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, drive, slope, mul, add, lmax = convertArgsToLists(self._in_fader, drive, slope, mul, add)
        self._base_objs = [Disto_base(wrap(in_fader,i), wrap(drive,i), wrap(slope,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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
 
    def setDrive(self, x):
        """
        Replace the `drive` attribute.
        
        Parameters:

        x : float or PyoObject
            New `drive` attribute.

        """
        self._drive = x
        x, lmax = convertArgsToLists(x)
        [obj.setDrive(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        """
        Replace the `slope` attribute.
        
        Parameters:

        x : float or PyoObject
            New `slope` attribute.

        """
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0., 1., 'lin', 'drive', self._drive),
                        SLMap(0., 0.999, 'lin', 'slope', self._slope),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None:
            title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Disto_demo.py")
    demo = Call_example(demo)

    def args():
        return("Disto(input, drive=.75, slope=.5, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def drive(self):
        """float or PyoObject. Amount of distortion.""" 
        return self._drive
    @drive.setter
    def drive(self, x): self.setDrive(x)

    @property
    def slope(self):
        """float or PyoObject. Slope of the lowpass filter.""" 
        return self._slope
    @slope.setter
    def slope(self, x): self.setSlope(x)

class Clip(PyoObject):
    """
    Clips a signal to a predefined limit.
    
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to process.
    min : float or PyoObject, optional
        Minimum possible value. Defaults to -1.
    max : float or PyoObject, optional
        Maximum possible value. Defaults to 1.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.

    Attributes:
    
    input : PyoObject. Input signal to filter.
    min : float or PyoObject. Minimum possible value.
    max : float or PyoObject. Maximum possible value.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    >>> lfoup = Sine(freq=.25, mul=.48, add=.5)
    >>> lfodown = 0 - lfoup
    >>> c = Clip(a, min=lfodown, max=lfoup, mul=.5).out()

    """
    def __init__(self, input, min=-1.0, max=1.0, mul=1, add=0):
        self._input = input
        self._min = min
        self._max = max
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [Clip_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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
 
    def setMin(self, x):
        """
        Replace the `min` attribute.
        
        Parameters:

        x : float or PyoObject
            New `min` attribute.

        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.
        
        Parameters:

        x : float or PyoObject
            New `max` attribute.

        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(-1., 0., 'lin', 'min', self._min),
                        SLMap(0., 1., 'lin', 'max', self._max),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None:
            title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Clip_demo.py")
    demo = Call_example(demo)

    def args():
        return("Clip(input, min=-1, max=1, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def min(self):
        """float or PyoObject. Minimum possible value.""" 
        return self._min
    @min.setter
    def min(self, x): self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Minimum possible value.""" 
        return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class Delay(PyoObject):
    """
    Sweepable recursive delay.
    
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to delayed.
    delay : float or PyoObject, optional
        Delay time in seconds. Defaults to 0.25.
    feedback : float or PyoObject, optional
        Amount of output signal sent back into the delay line.
         Defaults to 0.
    maxdelay : float, optional
        Maximum delay length in seconds. Available only at initialization. 
        Defaults to 1.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setDelay(x) : Replace the `delay` attribute.
    setFeedback(x) : Replace the `feedback` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to delayed.
    delay : float or PyoObject. Delay time in seconds.
    feedback : float or PyoObject. Amount of output signal sent back 
        into the delay line.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    >>> d = Delay(a, delay=.2, feedback=.7, mul=.5).out()

    """
    def __init__(self, input, delay=0.25, feedback=0, maxdelay=1, mul=1, add=0):
        self._input = input
        self._delay = delay
        self._feedback = feedback
        self._maxdelay = maxdelay
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, delay, feedback, maxdelay, mul, add, lmax = convertArgsToLists(self._in_fader, delay, feedback, maxdelay, mul, add)
        self._base_objs = [Delay_base(wrap(in_fader,i), wrap(delay,i), wrap(feedback,i), wrap(maxdelay,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        
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

    def setDelay(self, x):
        """
        Replace the `delay` attribute.
        
        Parameters:

        x : float or PyoObject
            New `delay` attribute.

        """
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.
        
        Parameters:

        x : float or PyoObject
            New `feedback` attribute.

        """
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0.001, self._maxdelay, 'log', 'delay',  self._delay),
                        SLMap(0., 1., 'lin', 'feedback', self._feedback),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None:
            title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Delay_demo.py")
    demo = Call_example(demo)

    def args():
        return("Delay(input, delay=0.25, feedback=0, maxdelay=1, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to delayed.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
 
    @property
    def delay(self):
        """float or PyoObject. Delay time in seconds.""" 
        return self._delay
    @delay.setter
    def delay(self, x): self.setDelay(x)

    @property
    def feedback(self):
        """float or PyoObject. Amount of output signal sent back into the delay line.""" 
        return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)

class Waveguide(PyoObject):
    """
    A waveguide model consisting of one delay-line with a simple 
    lowpass filtering and lagrange interpolation.
    
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to delayed.
    freq : float or PyoObject, optional
        Frequency, in cycle per second, of the waveguide (i.e. the inverse 
        of delay time). Defaults to 100.
    dur : float or PyoObject, optional
        Duration, in seconds, for the waveguide to drop 40 dB below it's 
        maxima. Defaults to 10.
    minfreq : float, optional
        Minimum possible frequency, used to initialized delay length. 
        Available only at initialization. Defaults to 20.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setDur(x) : Replace the `dur` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to delayed.
    freq : float or PyoObject. Frequency in cycle per second.
    dur : float or PyoObject. Resonance duration in seconds.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = LinTable([(0,0), (2,1), (5,0), (8191,0)])
    >>> met = Metro().play()
    >>> pick = TrigEnv(met, t, 1)
    >>> w = Waveguide(pick, freq=[200,400], dur=20, minfreq=20, mul=.5).out()

    """
    def __init__(self, input, freq=100, dur=10, minfreq=20, mul=1, add=0):
        self._input = input
        self._freq = freq
        self._dur = dur
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, dur, minfreq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, dur, minfreq, mul, add)
        self._base_objs = [Waveguide_base(wrap(in_fader,i), wrap(freq,i), wrap(dur,i), wrap(minfreq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        
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

    def setDur(self, x):
        """
        Replace the `dur` attribute.
        
        Parameters:

        x : float or PyoObject
            New `dur` attribute.

        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(10, 500., 'log', 'freq',  self._freq),
                        SLMapDur(self._dur),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None:
            title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Waveguide_demo.py")
    demo = Call_example(demo)

    def args():
        return("Waveguide(input, freq=100, dur=10, minfreq=20, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to delayed.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
 
    @property
    def freq(self):
        """float or PyoObject. Frequency in cycle per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def dur(self):
        """float or PyoObject. Resonance duration in seconds.""" 
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

class Freeverb(PyoObject):
    """
    Jezar's Freeverb.
    
    Freeverb is a reverb unit generator based on Jezar's public domain 
    C++ sources, composed of eight parallel comb filters, followed by four 
    allpass units in series. Filters on each stream are slightly detuned 
    in order to create multi-channel effects.
        
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to process.
    size : float or PyoObject, optional
        Controls the length of the reverb,  between 0 and 1. A higher 
        value means longer reverb. Defaults to 0.5.
    damp : float or PyoObject, optional
        High frequency attenuation, between 0 and 1. A higher value 
        will result in a faster decay of the high frequency range. 
        Defaults to 0.5.
    bal : float or PyoObject, optional
        Balance between wet and dry signal, between 0 and 1. 0 means no 
        reverb. Defaults to 0.5.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setSize(x) : Replace the `size` attribute.
    setDamp(x) : Replace the `damp` attribute.
    setBal(x) : Replace the `bal` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to process.
    size : float or PyoObject. Room size.
    damp : float or PyoObject. High frequency damping.
    bal : float or PyoObject. Balance between wet and dry signal.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(DEMOS_PATH + "/transparent.aif", loop=True)
    >>> b = Freeverb(a, size=.8, damp=.9, bal=.3).out()

    """
    def __init__(self, input, size=.5, damp=.5, bal=.5, mul=1, add=0):
        self._input = input
        self._size = size
        self._damp = damp
        self._bal = bal
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, size, damp, bal, mul, add, lmax = convertArgsToLists(self._in_fader, size, damp, bal, mul, add)
        self._base_objs = [Freeverb_base(wrap(in_fader,i), wrap(size,i), wrap(damp,i), wrap(bal,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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
 
    def setSize(self, x):
        """
        Replace the `size` attribute.
        
        Parameters:

        x : float or PyoObject
            New `size` attribute.

        """
        self._size = x
        x, lmax = convertArgsToLists(x)
        [obj.setSize(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDamp(self, x):
        """
        Replace the `damp` attribute.
        
        Parameters:

        x : float or PyoObject
            New `damp` attribute.

        """
        self._damp = x
        x, lmax = convertArgsToLists(x)
        [obj.setDamp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setBal(self, x):
        """
        Replace the `bal` attribute.
        
        Parameters:

        x : float or PyoObject
            New `bal` attribute.

        """
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setMix(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0., 1., 'lin', 'size',  self._size),
                        SLMap(0., 1., 'lin', 'damp',  self._damp),
                        SLMap(0., 1., 'lin', 'bal',  self._bal),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None:
            title = self.__class__.__name__
        win.title(title)

    def demo():
        execfile(DEMOS_PATH + "/Freeverb_demo.py")
    demo = Call_example(demo)

    def args():
        return("Freeverb(input, size=.5, damp=.5, bal=.5, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def size(self):
        """float or PyoObject. Room size.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def damp(self):
        """float or PyoObject. High frequency damping.""" 
        return self._damp
    @damp.setter
    def damp(self, x): self.setDamp(x)

    @property
    def bal(self):
        """float or PyoObject. Balance between wet and dry signal.""" 
        return self._bal
    @bal.setter
    def bal(self, x): self.setBal(x)

class Compress(PyoObject):
    """
    Process that reduces the dynamic range of an audio signal.
    
    Parent class: PyoObject
    
    Parameters:

    input : PyoObject
        Input signal to filter.
    thresh : float or PyoObject, optional
        Level, expressed in dB, above which the signal is reduced. 
        Reference level is 0dB. Defaults to -20.
    ratio : float or PyoObject, optional
        Determines the input/output ratio for signals above the 
        threshold. Defaults to 2.
    risetime : float or PyoObject, optional
        Used in amplitude follower, time to reach upward value in 
        seconds. Defaults to 0.005.
    falltime : float or PyoObject, optional
        Used in amplitude follower, time to reach downward value in 
        seconds. Defaults to 0.05.
        
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setThresh(x) : Replace the `thresh` attribute.
    setRatio(x) : Replace the `ratio` attribute.
    setRiseTime(x) : Replace the `risetime` attribute.
    setFallTime(x) : Replace the `falltime` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    thresh : float or PyoObject. Level above which the signal is reduced.
    ratio : float or PyoObject. in/out ratio for signals above the threshold.
    risetime : float or PyoObject. Time to reach upward value in seconds.
    falltime : float or PyoObject. Time to reach downward value in seconds.
     
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer('pyodemos/transparent.aif', loop=True)
    >>> b = Compress(a, thresh=-30, ratio=4, risetime=.005, falltime=.1).out()
    
    """
    def __init__(self, input, thresh=-20, ratio=2, risetime=0.005, falltime=0.05, mul=1, add=0):
        self._input = input
        self._thresh = thresh
        self._ratio = ratio
        self._risetime = risetime
        self._falltime = falltime
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, thresh, ratio, risetime, falltime, mul, add, lmax = convertArgsToLists(self._in_fader, thresh, ratio, risetime, falltime, mul, add)
        self._base_objs = [Compress_base(wrap(in_fader,i), wrap(thresh,i), wrap(ratio,i), wrap(risetime,i), wrap(falltime,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def setThresh(self, x):
        """
        Replace the `thresh` attribute.
        
        Parameters:

        x : float or PyoObject
            New `thresh` attribute.

        """
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
 
    def setRatio(self, x):
        """
        Replace the `ratio` attribute.
        
        Parameters:

        x : float or PyoObject
            New `ratio` attribute.

        """
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
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

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(-90., 0., 'lin', 'thresh',  self._thresh),
                        SLMap(1., 10., 'lin', 'ratio',  self._ratio),
                        SLMap(0.001, .2, 'lin', 'risetime',  self._risetime),
                        SLMap(0.001, .2, 'lin', 'falltime',  self._falltime),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None:
            title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/Compress_demo.py")
    #demo = Call_example(demo)

    def args():
        return("Compress(input, thresh=-20, ratio=2, risetime=0.005, falltime=0.05, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def thresh(self):
        """float or PyoObject. Level above which the signal is reduced.""" 
        return self._thresh
    @thresh.setter
    def thresh(self, x): self.setThresh(x)

    @property
    def ratio(self):
        """float or PyoObject. in/out ratio for signals above the threshold.""" 
        return self._ratio
    @ratio.setter
    def ratio(self, x): self.setRatio(x)

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