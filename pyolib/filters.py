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
    Exponential portamento.
    
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

class BandSplit(PyoObject):
    """
    Split an input signal into multiple frequency bands.
    
    The signal will be filtered into `num` bands between `min` and `max` 
    frequencies. Each band will then be assigned to an independent audio 
    stream. Useful for multiband processing.

    Parent class: PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    num : int, optional
        Number of frequency bands created. Initialization time only. 
        Defaults to 6.
    min : float, optional
        Lowest frequency. Initialization time only. Defaults to 20.
    max : float, optional
        Highest frequency. Initialization time only. Defaults to 20000.
    q : float or PyoObject, optional
        Q of the filters, defined as bandwidth/cutoff. Should be 
        between 1 and 500. Defaults to 1.

    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    setQ(x) : Replace the `q` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    q : float or PyoObject. Q of the filters.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> lfos = Sine(freq=[.3,.4,.5,.6,.7,.8], mul=.5, add=.5)
    >>> n = Noise(.5)
    >>> a = BandSplit(n, num=6, min=250, max=4000, q=5, mul=lfos).out()

    """
    def __init__(self, input, num=6, min=20, max=20000, q=1, mul=1, add=0):
        self._input = input
        self._num = num
        self._min = min
        self._max = max
        self._q = q
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, q, lmax = convertArgsToLists(self._in_fader, q)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [BandSplitter_base(wrap(in_fader,i), num, min, max, wrap(q,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(num):
                self._base_objs.append(BandSplit_base(wrap(self._base_players,i), j, wrap(mul,j), wrap(add,j)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

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

    def setQ(self, x):
        """
        Replace the `q` attribute.
        
        Parameters:

        x : float or PyoObject
            new `q` attribute.
        
        """
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._base_players)]
                        
    def play(self):
        self._base_players = [obj.play() for obj in self._base_players]
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        self._base_players = [obj.play() for obj in self._base_players]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapQ(self._q), SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)
        
    def demo():
        execfile(DEMOS_PATH + "/BandSplit_demo.py")
    demo = Call_example(demo)

    def args():
        return("BandSplit(input, num=6, min=20, max=20000, q=1, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def q(self): 
        """float or PyoObject. Q of the filters."""
        return self._q
    @q.setter
    def q(self, x): self.setQ(x) 

class Hilbert(PyoObject):
    """
    Hilbert transform.
    
    Parent class : PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.

    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    
    Notes:
    
    Real and imaginary parts are two separated set of streams. 
    The user should call :
    
    Hilbert['real'] to retrieve the real part.
    Hilbert['imag'] to retrieve the imaginary part.
    
    Examples:
    
    >>> a = SfPlayer(DEMOS_PATH + "/accord.aif", loop=True).out()
    >>> b = Hilbert(a)
    >>> quad = Sine([250, 500], [0, .25])
    >>> mod1 = b['real'] * quad[0]
    >>> mod2 = b['imag'] * quad[1]
    >>> up = mod1 - mod2
    >>> down = mod1 + mod2
    >>> up.out()

    """
    def __init__(self, input, mul=1, add=0):
        self._real_dummy = None
        self._imag_dummy = None
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, lmax = convertArgsToLists(self._in_fader)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [HilbertMain_base(wrap(in_fader,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax2):
            self._base_objs.append(Hilbert_base(wrap(self._base_players,i), 0, wrap(mul,i), wrap(add,i)))
            self._base_objs.append(Hilbert_base(wrap(self._base_players,i), 1, wrap(mul,i), wrap(add,i)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

    def __getitem__(self, str):
        if str == 'real':
            if self._real_dummy == None:
                self._real_dummy = Dummy([obj for i, obj in enumerate(self._base_objs) if (i%2) == 0])
            return self._real_dummy
        if str == 'imag':
            if self._imag_dummy == None:
                self._imag_dummy = Dummy([obj for i, obj in enumerate(self._base_objs) if (i%2) == 1])
            return self._imag_dummy
     
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
                    
    def play(self):
        self._base_players = [obj.play() for obj in self._base_players]
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        self._base_players = [obj.play() for obj in self._base_players]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None):
        print "There is no control on Hilbert object."

    def demo():
        execfile(DEMOS_PATH + "/Hilbert_demo.py")
    demo = Call_example(demo)

    def args():
        return("Hilbert(input, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
