"""
PyoObjects to perform operations on PyoTableObjects.

PyoTableObjects are 1 dimension containers. They can be used to store 
audio samples or algorithmic sequences for future uses.

"""

"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from _core import *
from _maps import *
from types import SliceType
 
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
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setPhase(x) : Replace the `phase` attribute.
    setInterp(x) : Replace the `interp` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    freq : float or PyoObject, Frequency in cycles per second.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    interp : int {1, 2, 3, 4}, Interpolation method.
    
    See also: Phasor, Sine

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143])
    >>> a = Osc(table=t, freq=100).out()   
     
    """
    def __init__(self, table, freq=1000, phase=0, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._table = table
        self._freq = freq
        self._phase = phase
        self._interp = interp
        self._mul = mul
        self._add = add
        table, freq, phase, interp, mul, add, lmax = convertArgsToLists(table, freq, phase, interp, mul, add)
        self._base_objs = [Osc_base(wrap(table,i), wrap(freq,i), wrap(phase,i), wrap(interp,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['table', 'freq', 'phase', 'interp', 'mul', 'add']

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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.
        
        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.
        
        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapFreq(self._freq),
                          SLMapPhase(self._phase),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

class OscLoop(PyoObject):
    """
    A simple oscillator with feedback reading a waveform table.

    OscLoop reads a waveform table with linear interpolation and feedback control.
    The oscillator output, multiplied by `feedback`, is added to the position
    increment and can be used to control the brightness of the oscillator.
    
    
    Parent class: PyoObject

    Parameters:

    table : PyoTableObject
        Table containing the waveform samples.
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1000.
    feedback : float or PyoObject, optional
        Amount of the output signal added to position increment, between 0 and 1. 
        Controls the brightness. Defaults to 0.

    Methods:

    setTable(x) : Replace the `table` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setFeedback(x) : Replace the `feedback` attribute.

    Attributes:

    table : PyoTableObject. Table containing the waveform samples.
    freq : float or PyoObject, Frequency in cycles per second.
    feedback : float or PyoObject, Brightness control.

    See also: Osc, SineLoop

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143])
    >>> lfo = Sine(.1, 0, .05, .05)
    >>> a = OscLoop(table=t, freq=100, feedback=lfo).out()   

    """
    def __init__(self, table, freq=1000, feedback=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._table = table
        self._freq = freq
        self._feedback = feedback
        self._mul = mul
        self._add = add
        table, freq, feedback, mul, add, lmax = convertArgsToLists(table, freq, feedback, mul, add)
        self._base_objs = [OscLoop_base(wrap(table,i), wrap(freq,i), wrap(feedback,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['table', 'freq', 'feedback', 'mul', 'add']

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

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        Parameters:

        x : float or PyoObject
            new `feedback` attribute.

        """
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapFreq(self._freq),
                          SLMap(0, 1, "lin", "feedback", self._feedback),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    def feedback(self): 
        """float or PyoObject. Brightness control.""" 
        return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)

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
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setLoop(x) : Replace the `loop` attribute.
    setInterp(x) : Replace the `interp` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    freq : float or PyoObject, Frequency in cycles per second.
    loop : int, Looping mode.
    interp : int {1, 2, 3, 4}, Interpolation method.

    Notes:
    
    TableRead will sends a trigger signal at the end of the playback if 
    loop is off or any time it wraps around if loop is on. User can 
    retreive the trigger streams by calling obj['trig']:
    
    >>> tabr = TableRead(SNDS_PATH + "/transparent.aif").out()
    >>> trig = TrigRand(tab['trig'])
    
    See also: Osc

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH + '/transparent.aif')
    >>> a = TableRead(table=snd, freq=snd.getRate(), loop=0).out()   
     
    """
    def __init__(self, table, freq=1, loop=0, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._table = table
        self._freq = freq
        self._loop = loop
        self._interp = interp
        self._mul = mul
        self._add = add
        table, freq, loop, interp, mul, add, lmax = convertArgsToLists(table, freq, loop, interp, mul, add)
        self._base_objs = [TableRead_base(wrap(table,i), wrap(freq,i), wrap(loop,i), wrap(interp,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        self._trig_objs = [TableReadTrig_base(obj) for obj in self._base_objs]

    def __dir__(self):
        return ['table', 'freq', 'loop', 'interp', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._trig_objs:
            obj.deleteStream()
            del obj
            
    def __getitem__(self, i):
        if i == 'trig':
            return self._trig_objs
        
        if type(i) == SliceType:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         

    def play(self):
        self._base_objs = [obj.play() for obj in self._base_objs]
        self._trig_objs = [obj.play() for obj in self._trig_objs]
        return self

    def out(self, chnl=0, inc=1):
        self._trig_objs = [obj.play() for obj in self._trig_objs]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self
        
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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.
        
        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.
        
        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapFreq(self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

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
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setEnv(x) : Replace the `emv` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setFrac(x) : Replace the `frac` attribute.
    setPhase(x) : Replace the `phase` attribute.
    setInterp(x) : Replace the `interp` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    env : PyoTableObject. Table containing the envelope samples.
    freq : float or PyoObject, Frequency in cycles per second.
    frac : float or PyoObject, Fraction of the period assigned to waveform.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    interp : int {1, 2, 3, 4}, Interpolation method.
    
    See also: Osc

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> w = HarmTable([1,0,.33,0,2,0,.143,0,.111])
    >>> e = HannTable()
    >>> lfo = Sine(.15, mul=.2, add=.5)
    >>> a = Pulsar(table=w, env=e, freq=80, frac=lfo, mul=.25).out()
     
    """
    def __init__(self, table, env, freq=100, frac=0.5, phase=0, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._table = table
        self._env = env
        self._freq = freq
        self._frac = frac
        self._phase = phase
        self._interp = interp
        self._mul = mul
        self._add = add
        table, env, freq, frac, phase, interp, mul, add, lmax = convertArgsToLists(table, env, freq, frac, phase, interp, mul, add)
        self._base_objs = [Pulsar_base(wrap(table,i), wrap(env,i), wrap(freq,i), wrap(frac,i), wrap(phase,i), wrap(interp,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['table', 'env', 'freq', 'frac', 'phase', 'interp', 'mul', 'add']

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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.
        
        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.
        
        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapFreq(self._freq),
                          SLMap(0., 1., 'lin', 'frac', self._frac),
                          SLMapPhase(self._phase),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

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

    Attributes:
    
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
        PyoObject.__init__(self)
        self._table = table
        self._index = index
        self._mul = mul
        self._add = add
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [Pointer_base(wrap(table,i), wrap(index,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['table', 'index', 'mul', 'add']

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
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

class Lookup(PyoObject):
    """
    Uses samples table to do waveshaping on an audio signal.
    
    The input signal amplitudes, between -1 and 1 are scaled between 
    0 and the table size and used as position index in the table to 
    apply waveshaping on the input signal.  
    
    Parent class: PyoObject
    
    Parameters:
    
    table : PyoTableObject
        Table containing the transfert function.
    index : PyoObject
        Audio signal, between -1 and 1, internally converted to be
        used as the index position in the table.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setIndex(x) : Replace the `index` attribute.

    table : PyoTableObject. Table containing the transfert function.
    index : PyoObject. Audio input used as the table index.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(freq=[.15,.2], mul=.45, add=.5)
    >>> a = Sine(freq=[100,150], mul=lfo)
    >>> t = CosTable([(0,-1),(3072,-0.85),(4096,0),(5520,.85),(8192,1)])
    >>> b = Lookup(table=t, index=a, mul=1.-lfo).out()

    """
    def __init__(self, table, index, mul=1, add=0):
        PyoObject.__init__(self)
        self._table = table
        self._index = index
        self._mul = mul
        self._add = add
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [Lookup_base(wrap(table,i), wrap(index,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['table', 'index', 'mul', 'add']

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
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

    @property
    def table(self):
        """PyoTableObject. Table containing the transfert function.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def index(self):
        """PyoObject. Audio input used as the table index.""" 
        return self._index
    @index.setter
    def index(self, x): self.setIndex(x)

class TableRec(PyoObject):
    """
    TableRec is for writing samples into a previously created NewTable.
     
    See `NewTable` to create an empty table.

    The play method is not called at the object creation time. It starts
    the recording into the table until the table is full. Calling the 
    play method again restarts the recording and overwrites previously
    recorded samples.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal to write in the table.
    table : PyoTableObject
        The table where to write samples.
    fadetime : float, optional
        Fade time at the beginning and the end of the recording 
        in seconds. Defaults to 0.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setTable(x) : Replace the `table` attribute.
    play() : Start the recording at the beginning of the table.
    stop() : Stop the recording. Otherwise, record through the 
        end of the table.

    Attributes:
    
    input : PyoObject. Audio signal to write in the table.
    table : PyoTableObject. The table where to write samples.
    
    Notes:

    The out() method is bypassed. TableRec returns no signal.
    
    TableRec has no `mul` and `add` attributes.
    
    TableRec will sends a trigger signal at the end of the recording. 
    User can retreive the trigger streams by calling obj['trig']. In
    this example, the recorded table will be read automatically after
    a recording:
    
    >>> a = Input(0)
    >>> t = NewTable(length=1, chnls=1)
    >>> rec = TableRec(a, table=t, fadetime=0.01)
    >>> tr = TrigEnv(rec['trig'], table=t, dur=1).out()

    See also: NewTable
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = NewTable(length=2, chnls=1)
    >>> a = Input(0)
    >>> b = TableRec(a, t, .01)
    >>> c = Osc(t, [t.getRate(), t.getRate()*.99]).out()
    >>> # to record in the empty table, call:
    >>> # b.play()
    
    """
    def __init__(self, input, table, fadetime=0):
        PyoObject.__init__(self)
        self._input = input
        self._table = table
        self._in_fader = InputFader(input)
        in_fader, table, fadetime, lmax = convertArgsToLists(self._in_fader, table, fadetime)
        self._base_objs = [TableRec_base(wrap(in_fader,i), wrap(table,i), wrap(fadetime,i)) for i in range(len(table))]
        self._trig_objs = [TableRecTrig_base(obj) for obj in self._base_objs]

    def __dir__(self):
        return ['input', 'table', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._trig_objs:
            obj.deleteStream()
            del obj

    def __getitem__(self, i):
        if i == 'trig':
            return self._trig_objs
        
        if type(i) == SliceType:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         

    def play(self):
        self._base_objs = [obj.play() for obj in self._base_objs]
        self._trig_objs = [obj.play() for obj in self._trig_objs]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self

    def out(self, chnl=0, inc=1):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

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

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : NewTable
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
      
    @property
    def input(self):
        """PyoObject. Audio signal to write in the table.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def table(self):
        """PyoTableObject. The table where to write samples."""
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

class TableMorph(PyoObject):
    """
    Morphs between multiple PyoTableObject.
     
    Uses an index into a list of PyoTableObjects to morph between adjacent 
    tables in the list. This morphed function is written into the `table`
    object at the beginning of each buffer size. The tables in the list and 
    the resulting table must be equal in size.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Morphing index between 0 and 1. 0 is the first table in the list 
        and 1 is the last.
    table : NewTable
        The table where to write morphed waveform.
    sources : list of PyoTableObject
        List of tables to interpolate from.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setTable(x) : Replace the `table` attribute.
    setSources(x) : Replace the `sources` attribute.

    Attributes:
    
    input : PyoObject. Morphing index between 0 and 1.
    table : NewTable. The table where to write samples.
    sources : list of PyoTableObject. List of tables to interpolate from.
    
    Notes:

    The out() method is bypassed. TableMorph returns no signal.
    
    TableMorph has no `mul` and `add` attributes.
 
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t1 = HarmTable([1,.5,.33,.25,.2,.167,.143,.125,.111])
    >>> t2 = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> t3 = NewTable(length=8192./s.getSamplingRate(), chnls=1)
    >>> lfo = Sine(.25, 0, .5, .5)
    >>> mor = TableMorph(lfo, t3, [t1,t2])
    >>> osc = Osc(t3, freq=200, mul=.5).out()
    
    """
    def __init__(self, input, table, sources):
        PyoObject.__init__(self)
        self._input = input
        self._table = table
        self._sources = sources
        self._in_fader = InputFader(input)
        in_fader, table, lmax = convertArgsToLists(self._in_fader, table)
        self._base_sources = [source[0] for source in sources]
        self._base_objs = [TableMorph_base(wrap(in_fader,i), wrap(table,i), self._base_sources) for i in range(len(table))]

    def __dir__(self):
        return ['input', 'table', 'sources', 'mul', 'add']

    def out(self, chnl=0, inc=1):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

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

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : NewTable
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSources(self, x):
        """
         Replace the `sources` attribute.
        
        Parameters:

        x : list of PyoTableObject
            new `sources` attribute.
              
        """
        self._sources = x
        self._base_sources = [source[0] for source in x]
        [obj.setSources(self._base_sources) for i, obj in enumerate(self._base_objs)]
        
    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
      
    @property
    def input(self):
        """PyoObject. Morphing index between 0 and 1.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def table(self):
        """NewTable. The table where to write samples."""
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def sources(self):
        """list of PyoTableObject. List of tables to interpolate from."""
        return self._sources
    @sources.setter
    def sources(self, x): self.setSources(x)

class Granulator(PyoObject):
    """
    Granular synthesis generator.

    Parent class: PyoObject
    
    Parameters:

    table : PyoTableObject
        Table containing the waveform samples.
    env : PyoTableObject
        Table containing the grain envelope.
    pitch : float or PyoObject, optional
        Overall pitch of the granulator. This value transpose the 
        pitch of all grains. Defaults to 1.
    pos : float or PyoObject, optional
        Pointer position, in samples, in the waveform table. Each 
        grain sampled the current value of this stream at the beginning 
        of its envelope and hold it until the end of the grain. 
        Defaults to 0.
    dur : float or PyoObject, optional
        Duration, in seconds, of the grain. Each grain sampled the 
        current value of this stream at the beginning of its envelope 
        and hold it until the end of the grain. Defaults to 0.1.
    grains : int, optional
        Number of grains. Defaults to 8.
    basedur : float, optional
        Base duration used to calculate the speed of the pointer to 
        read the grain at its original pitch. By changing the value of 
        the `dur` parameter, transposition per grain can be generated.
        Defaults to 0.1.
    
    Methods:
    
    setTable(x) : Replace the `table` attribute.
    setEnv(x) : Replace the `env` attribute.
    setPitch(x) : Replace the `pitch` attribute.
    setPos(x) : Replace the `pos` attribute.
    setDur(x) : Replace the `dur` attribute.
    setGrains(x) : Replace the `grains` attribute.
    setBaseDur(x) : Replace the `basedur` attribute.
    
    Attributes:
    
    table : PyoTableObject. The table where to write samples.
    env : PyoTableObject. Table containing the grain envelope.
    pitch : float or PyoObject. Overall pitch of the granulator.
    pos : float or PyoObject. Position of the pointer in the sound table.
    dur : float or PyoObject. Duration, in seconds, of the grain.
    grains : int. Number of grains.
    basedur : float. Duration to read the grain at its original pitch.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH + "/transparent.aif")
    >>> env = HannTable()
    >>> pos = Phasor(snd.getRate()*.25, 0, snd.getSize())
    >>> dur = Noise(.001, .1)
    >>> g = Granulator(snd, env, [1, 1.001], pos, dur, 24, mul=.1).out()

    """
    def __init__(self, table, env, pitch=1, pos=0, dur=.1, grains=8, basedur=.1, mul=1, add=0):
        PyoObject.__init__(self)
        self._table = table
        self._env = env
        self._pitch = pitch
        self._pos = pos
        self._dur = dur
        self._grains = grains
        self._basedur = basedur
        self._mul = mul
        self._add = add
        table, env, pitch, pos, dur, grains, basedur, mul, add, lmax = convertArgsToLists(table, env, pitch, 
                                                                        pos, dur, grains, basedur, mul, add)
        self._base_objs = [Granulator_base(wrap(table,i), wrap(env,i), wrap(pitch,i), wrap(pos,i), wrap(dur,i), 
                          wrap(grains,i), wrap(basedur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['table', 'env', 'pitch', 'pos', 'dur', 'grains', 'basedur', 'mul', 'add']

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

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pitch` attribute.
        
        """
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pos` attribute.
        
        """
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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

    def setGrains(self, x):
        """
        Replace the `grains` attribute.
        
        Parameters:

        x : int
            new `grains` attribute.
        
        """
        self._grains = x
        x, lmax = convertArgsToLists(x)
        [obj.setGrains(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setBaseDur(self, x):
        """
        Replace the `basedur` attribute.
        
        Parameters:

        x : float
            new `basedur` attribute.
        
        """
        self._basedur = x
        x, lmax = convertArgsToLists(x)
        [obj.setBaseDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0.1, 2., 'lin', 'pitch', self._pitch),
                          SLMap(0.01, 1., 'lin', 'dur', self._dur),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

    @property
    def table(self):
        """PyoTableObject. The table where to write samples."""
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the grain envelope."""
        return self._env
    @env.setter
    def env(self, x): self.setEnv(x)

    @property
    def pitch(self):
        """float or PyoObject. Overall pitch of the granulator."""
        return self._pitch
    @pitch.setter
    def pitch(self, x): self.setPitch(x)

    @property
    def pos(self):
        """float or PyoObject. Position of the pointer in the sound table."""
        return self._pos
    @pos.setter
    def pos(self, x): self.setPos(x)

    @property
    def dur(self):
        """float or PyoObject. Duration, in seconds, of the grain."""
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

    @property
    def grains(self):
        """int. Number of grains."""
        return self._grains
    @grains.setter
    def grains(self, x): self.setGrains(x)

    @property
    def basedur(self):
        """float. Duration to read the grain at its original pitch."""
        return self._basedur
    @basedur.setter
    def basedur(self, x): self.setBaseDur(x)

