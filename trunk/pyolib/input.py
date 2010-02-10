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
 
class Osc(PyoObject):
    """
    A simple oscillator with linear interpolation reading a waveform table.
    
    Parent class: PyoObject
    
    Parameters:
    
    table : PyoTableObject
        Table containing the waveform samples.
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1000.
    phase : float or PyoObject, optional
        Phase of sampling, expressed as a fraction of a cycle (0 to 1). 
        Defaults to 0.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setPhase(x) : Replace the `phase` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    freq : float or PyoObject, Frequency in cycles per second.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    
    See also: Phasor, Sine

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143])
    >>> a = Osc(table=t, freq=100).out()   
     
    """
    def __init__(self, table, freq=1000, phase=0, mul=1, add=0):
        self._table = table
        self._freq = freq
        self._phase = phase
        self._mul = mul
        self._add = add
        table, freq, phase, mul, add, lmax = convertArgsToLists(table, freq, phase, mul, add)
        self._base_objs = [Osc_base(wrap(table,i), wrap(freq,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : PyoTableObject
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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
        execfile(DEMOS_PATH + "/Osc_demo.py")
    demo = Call_example(demo)

    def args():
        return("Osc(table, freq=1000, phase=0, mul=1, add=0)")
    args = Print_args(args)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

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

class TableRead(PyoObject):
    """
    Simple waveform table reader.
    
    Read sampled sound from a table, with optional looping mode.
    
    Parent class: PyoObject
    
    Parameters:
    
    table : PyoTableObject
        Table containing the waveform samples.
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1.
    loop : int {0, 1}, optional
        Looping mode, 0 means off, 1 means on. 
        Defaults to 0.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setLoop(x) : Replace the `loop` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    freq : float or PyoObject, Frequency in cycles per second.
    loop : int, Looping mode.
    
    See also: Osc

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(DEMOS_PATH + '/transparent.aif')
    >>> a = TableRead(table=snd, freq=snd.getRate(), loop=0).out()   
     
    """
    def __init__(self, table, freq=1, loop=0, mul=1, add=0):
        self._table = table
        self._freq = freq
        self._loop = loop
        self._mul = mul
        self._add = add
        table, freq, loop, mul, add, lmax = convertArgsToLists(table, freq, loop, mul, add)
        self._base_objs = [TableRead_base(wrap(table,i), wrap(freq,i), wrap(loop,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : PyoTableObject
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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

    def setLoop(self, x):
        """
        Replace the `loop` attribute.
        
        Parameters:

        x : int {0, 1}
            new `loop` attribute.
        
        """
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapFreq(self._freq),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/TableRead_demo.py")
    #demo = Call_example(demo)

    def args():
        return("TableRead(table, freq=1, loop=0, mul=1, add=0)")
    args = Print_args(args)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def loop(self): 
        """int. Looping mode.""" 
        return self._loop
    @loop.setter
    def loop(self, x): self.setLoop(x)

class Pulsar(PyoObject):
    """
    Pulsar synthesis oscillator.
    
    Pulsar synthesis produces a train of sound particles called pulsars 
    that can make rhythms or tones, depending on the fundamental frequency 
    of the train. Varying the `frac` portion of the period assigned to the
    waveform and its following silence, but maintaining the overall pulsar 
    period gives an effect much like a band-pass filter.

    Parent class: PyoObject
    
    Parameters:
    
    table : PyoTableObject
        Table containing the waveform samples.
    env : PyoTableObject
        Table containing the envelope samples.
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 100.
    frac : float or PyoObject, optional
        Fraction of the whole period (0 -> 1) given to the waveform. 
        The rest will be filled with zeros. Defaults to 0.5.
    phase : float or PyoObject, optional
        Phase of sampling, expressed as a fraction of a cycle (0 to 1). 
        Defaults to 0.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setEnv(x) : Replace the `emv` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setFrac(x) : Replace the `frac` attribute.
    setPhase(x) : Replace the `phase` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    env : PyoTableObject. Table containing the envelope samples.
    freq : float or PyoObject, Frequency in cycles per second.
    frac : float or PyoObject, Fraction of the period assigned to waveform.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    
    See also: Osc

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> w = HarmTable([1,0,.33,0,2,0,.143,0,.111])
    >>> e = HannTable()
    >>> lfo = Sine(.15, mul=.2, add=.5)
    >>> a = Pulsar(table=w, env=e, freq=80, frac=lfo, mul=.25).out()
     
    """
    def __init__(self, table, env, freq=100, frac=0.5, phase=0, mul=1, add=0):
        self._table = table
        self._env = env
        self._freq = freq
        self._frac = frac
        self._phase = phase
        self._mul = mul
        self._add = add
        table, env, freq, frac, phase, mul, add, lmax = convertArgsToLists(table, env, freq, frac, phase, mul, add)
        self._base_objs = [Pulsar_base(wrap(table,i), wrap(env,i), wrap(freq,i), wrap(frac,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : PyoTableObject
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.
        
        Parameters:

        x : PyoTableObject
            new `env` attribute.
        
        """
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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

    def setFrac(self, x):
        """
        Replace the `frac` attribute.
        
        Parameters:

        x : float or PyoObject
            new `frac` attribute.
        
        """
        self._frac = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrac(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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
                        SLMap(0., 1., 'lin', 'frac', self._frac),
                        SLMapPhase(self._phase),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/Pulsar_demo.py")
    #demo = Call_example(demo)

    def args():
        return("Pulsar(table, env, freq=100, frac=0.5, phase=0, mul=1, add=0)")
    args = Print_args(args)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the envelope samples.""" 
        return self._env
    @env.setter
    def env(self, x): self.setEnv(x)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def frac(self):
        """float or PyoObject. Fraction of the period assigned to waveform.""" 
        return self._frac
    @frac.setter
    def frac(self, x): self.setFrac(x)

    @property
    def phase(self): 
        """float or PyoObject. Phase of sampling.""" 
        return self._phase
    @phase.setter
    def phase(self, x): self.setPhase(x)

class Pointer(PyoObject):
    """
    Table reader with control on the pointer position.
    
    Parent class: PyoObject
    
    Parameters:
    
    table : PyoTableObject
        Table containing the waveform samples.
    index : PyoObject
        Normalized position in the table between 0 and 1.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setIndex(x) : Replace the `index` attribute.

    table : PyoTableObject. Table containing the waveform samples.
    index : PyoObject. Pointer position in the table.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SndTable('pyodemos/transparent.aif')
    >>> p = Phasor(freq=t.getRate())
    >>> a = Pointer(table=t, index=p).out()

    """
    def __init__(self, table, index, mul=1, add=0):
        self._table = table
        self._index = index
        self._mul = mul
        self._add = add
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [Pointer_base(wrap(table,i), wrap(index,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : PyoTableObject
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.
        
        Parameters:

        x : PyoObject
            new `index` attribute.
        
        """
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/Pointer_demo.py")
    #demo = Call_example(demo)


    def args():
        return("Pointer(table, index, mul=1, add=0)")
    args = Print_args(args)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def index(self):
        """PyoObject. Index pointer position in the table.""" 
        return self._index
    @index.setter
    def index(self, x): self.setIndex(x)

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
