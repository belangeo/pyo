"""
Different kinds of audio filtering operations.
 
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
        Q of the filter, defined (for bandpass filters) as freq/bandwidth. 
        Should be between 1 and 500. Defaults to 1.
    type : int, optional
        Filter type. Five possible values :
            0 = lowpass (default)
            1 = highpass
            2 = bandpass
            3 = bandstop
            4 = allpass

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
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, type, mul, add)
        self._base_objs = [Biquad_base(wrap(in_fader,i), wrap(freq,i), wrap(q,i), wrap(type,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'freq', 'q', 'type', 'mul', 'add']

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
            0 = lowpass, 1 = highpass, 2 = bandpass, 3 = bandstop, 4 = allpass

        """
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapQ(self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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

class Biquadx(PyoObject):
    """
    A multi-stages sweepable general purpose biquadratic digital filter. 
    
    Biquadx is equivalent to a filter consisting of more layers of Biquad
    with the same arguments, serially connected. It is faster than using
    a large number of instances of the Biquad object, It uses less memory 
    and allows filters with sharper cutoff.

    Parent class : PyoObject

    Parameters:

    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Cutoff or center frequency of the filter. Defaults to 1000.
    q : float or PyoObject, optional
        Q of the filter, defined (for bandpass filters) as freq/bandwidth. 
        Should be between 1 and 500. Defaults to 1.
    type : int, optional
        Filter type. Five possible values :
            0 = lowpass (default)
            1 = highpass
            2 = bandpass
            3 = bandstop
            4 = allpass
    stages : int, optional
        The number of filtering stages in the filter stack. Defaults to 4.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setQ(x) : Replace the `q` attribute.
    setType(x) : Replace the `type` attribute.
    setType(x) : Replace the `stages` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Cutoff or center frequency of the filter.
    q : float or PyoObject. Q of the filter.
    type : int. Filter type.
    stages : int. The number of filtering stages.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.5)
    >>> lfo = Sine(freq=.25, mul=1000, add=1000)
    >>> f = Biquadx(a, freq=lfo, q=5, type=2).out()

    """
    def __init__(self, input, freq=1000, q=1, type=0, stages=4, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._stages = stages
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, stages, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, type, stages, mul, add)
        self._base_objs = [Biquadx_base(wrap(in_fader,i), wrap(freq,i), wrap(q,i), wrap(type,i), wrap(stages,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'freq', 'q', 'type', 'stages', 'mul', 'add']

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
            0 = lowpass, 1 = highpass, 2 = bandpass, 3 = bandstop, 4 = allpass

        """
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        """
        Replace the `stages` attribute.

        Parameters:

        x : int
            New `stages` attribute. 

        """
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapQ(self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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

    @property
    def stages(self):
        """int. The number of filtering stages.""" 
        return self._stages
    @stages.setter
    def stages(self, x): self.setStages(x)

class EQ(PyoObject):
    """
    Equalizer filter. 
    
    EQ is a biquadratic digital filter designed for equalization. It 
    provides peak/notch and lowshelf/highshelf filters for building 
    parametric equalizers.
    
    Parent class : PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Cutoff or center frequency of the filter. Defaults to 1000.
    q : float or PyoObject, optional
        Q of the filter, defined as freq/bandwidth. 
        Should be between 1 and 500. Defaults to 1.
    boost : float or PyoObject, optional
        Gain, expressed in dB, to add or remove at the center frequency. 
        Default to -3.
    type : int, optional
        Filter type. Three possible values :
            0 = peak/notch (default)
            1 = lowshelf
            2 = highshelf

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setQ(x) : Replace the `q` attribute.
    setBoost(x) : Replace the `boost` attribute.
    setType(x) : Replace the `type` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Cutoff or center frequency of the filter.
    q : float or PyoObject. Q of the filter.
    boost : float or PyoObject. Boost of the filter at center frequency.
    type : int. Filter type.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> amp = Fader(1, 1, mul=.3).play()
    >>> src = Noise(amp)
    >>> fr = Sine(.2, 0, 500, 1500)
    >>> boo = Sine(4, 0, 6)
    >>> out = EQ(src, freq=fr, q=1, boost=boo, type=0).out()

    """
    def __init__(self, input, freq=1000, q=1, boost=-3.0, type=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._q = q
        self._boost = boost
        self._type = type
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, q, boost, type, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, boost, type, mul, add)
        self._base_objs = [EQ_base(wrap(in_fader,i), wrap(freq,i), wrap(q,i), wrap(boost,i), wrap(type,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'freq', 'q', 'boost', 'type', 'mul', 'add']

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

    def setBoost(self, x):
        """
        Replace the `boost` attribute, expressed in dB.
        
        Parameters:

        x : float or PyoObject
            New `boost` attribute.

        """
        self._boost = x
        x, lmax = convertArgsToLists(x)
        [obj.setBoost(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.
        
        Parameters:

        x : int
            New `type` attribute. 
            0 = peak, 1 = lowshelf, 2 = highshelf

        """
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapQ(self._q), 
                          SLMap(-40.0, 40.0, "lin", "boost", self._boost), 
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
    def boost(self):
        """float or PyoObject. Boost factor of the filter.""" 
        return self._boost
    @boost.setter
    def boost(self, x): self.setBoost(x)

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
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [Tone_base(wrap(in_fader,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'freq', 'mul', 'add']
        
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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
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
        PyoObject.__init__(self)
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, init, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, init, mul, add)
        self._base_objs = [Port_base(wrap(in_fader,i), wrap(risetime,i), wrap(falltime,i), wrap(init,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'risetime', 'falltime', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.001, 10., 'lin', 'risetime', self._risetime),
                          SLMap(0.001, 10., 'lin', 'falltime', self._falltime)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [DCBlock_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'mul', 'add']
        
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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class BandSplit(PyoObject):
    """
    Splits an input signal into multiple frequency bands.
    
    The input signal will be separated into `num` bands between `min` 
    and `max` frequencies. Each band will then be assigned to an 
    independent audio stream. Useful for multiband processing.

    Parent class: PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    num : int, optional
        Number of frequency bands created. Available at initialization 
        time only. Defaults to 6.
    min : float, optional
        Lowest frequency. Available at initialization time only. 
        Defaults to 20.
    max : float, optional
        Highest frequency. Available at initialization time only. 
        Defaults to 20000.
    q : float or PyoObject, optional
        Q of the filters, defined as center frequency / bandwidth. 
        Should be between 1 and 500. Defaults to 1.

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
        PyoObject.__init__(self)
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

    def __dir__(self):
        return ['input', 'q', 'mul', 'add']

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
                        
    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapQ(self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
    get(identifier, all) : Return the first sample of the current 
        buffer as a float.

    Attributes:
    
    input : PyoObject. Input signal to filter.
    
    Notes:
    
    Real and imaginary parts are two separated set of streams. 
    The user should call :
    
    Hilbert['real'] to retrieve the real part.
    Hilbert['imag'] to retrieve the imaginary part.
    
    Examples:
    
    >>> a = SfPlayer(SNDS_PATH + "/accord.aif", loop=True).out(0)
    >>> b = Hilbert(a)
    >>> quad = Sine([250, 500], [0, .25])
    >>> mod1 = b['real'] * quad[0]
    >>> mod2 = b['imag'] * quad[1]
    >>> up = mod1 - mod2
    >>> down = mod1 + mod2
    >>> up.out(1)

    """
    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
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

    def __dir__(self):
        return ['input', 'mul', 'add']

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

    def get(self, identifier="real", all=False):
        """
        Return the first sample of the current buffer as a float.
        
        Can be used to convert audio stream to usable Python data.
        
        "real" or "imag" must be given to `identifier` to specify
        which stream to get value from.
        
        Parameters:

            identifier : string {"real", "imag"}
                Address string parameter identifying audio stream.
                Defaults to "real".
            all : boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.
                 
        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]
 
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
                    
    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Allpass(PyoObject):
    """
    Delay line based allpass filter.
    
    Allpass is based on the combination of feedforward and feedback comb
    filter. This kind of filter is often used in simple digital reverb
    implementations.

    Parent class : PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.
    delay : float or PyoObject, optional
        Delay time in seconds. Defaults to 0.01.
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

    input : PyoObject. Input signal to process.
    delay : float or PyoObject. Delay time in seconds.
    feedback : float or PyoObject. Amount of output signal sent back 
        into the delay line.

    Examples:

    >>> # SIMPLE REVERB
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=0.25).mix(2).out()
    >>> b1 = Allpass(a, delay=[.0204,.02011], feedback=0.25)
    >>> b2 = Allpass(b1, delay=[.06653,.06641], feedback=0.31)
    >>> b3 = Allpass(b2, delay=[.035007,.03504], feedback=0.4)
    >>> b4 = Allpass(b3, delay=[.023021 ,.022987], feedback=0.55)
    >>> c1 = Tone(b1, 5000, mul=0.2).out()
    >>> c2 = Tone(b2, 3000, mul=0.2).out()
    >>> c3 = Tone(b3, 1500, mul=0.2).out()
    >>> c4 = Tone(b4, 500, mul=0.2).out()
    
    """
    def __init__(self, input, delay=0.01, feedback=0, maxdelay=1, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._delay = delay
        self._feedback = feedback
        self._maxdelay = maxdelay
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, delay, feedback, maxdelay, mul, add, lmax = convertArgsToLists(self._in_fader, delay, feedback, maxdelay, mul, add)
        self._base_objs = [Allpass_base(wrap(in_fader,i), wrap(delay,i), wrap(feedback,i), wrap(maxdelay,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'delay', 'feedback', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.001, self._maxdelay, 'log', 'delay',  self._delay),
                          SLMap(0., 1., 'lin', 'feedback', self._feedback),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
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

class Allpass2(PyoObject):
    """
    Second-order phase shifter allpass. 
    
    This kind of filter is used in phaser implementation. The signal
    of this filter, when added to original sound, creates a notch in
    the spectrum at frequencies that are in phase opposition.
    
    Parent class : PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Center frequency of the filter. Defaults to 1000.
    bw : float or PyoObject, optional
        Bandwidth of the filter in Hertz. Defaults to 100.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setBw(x) : Replace the `bw` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Center frequency of the filter.
    bw : float or PyoObject. Bandwidth of the filter.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> # 3 STAGES PHASER
    >>> a = Noise(.025).mix(2).out()
    >>> blfo = Sine(freq=.1, mul=250, add=500)
    >>> b = Allpass2(a, freq=blfo, bw=125).out()
    >>> clfo = Sine(freq=.14, mul=500, add=1000)
    >>> c = Allpass2(b, freq=clfo, bw=350).out()
    >>> dlfo = Sine(freq=.17, mul=1000, add=2500)
    >>> d = Allpass2(c, freq=dlfo, bw=800).out()

    """
    def __init__(self, input, freq=1000, bw=100, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._bw = bw
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, bw, mul, add, lmax = convertArgsToLists(self._in_fader, freq, bw, mul, add)
        self._base_objs = [Allpass2_base(wrap(in_fader,i), wrap(freq,i), wrap(bw,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'freq', 'bw', 'mul', 'add']

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

    def setBw(self, x):
        """
        Replace the `bw` attribute.
        
        Parameters:

        x : float or PyoObject
            New `bw` attribute.

        """
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBw(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(10, 1000, "lin", "bw", self._bw), 
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def bw(self):
        """float or PyoObject. Bandwidth of the filter.""" 
        return self._bw
    @bw.setter
    def bw(self, x): self.setBw(x)

class Phaser(PyoObject):
    """
    Multi-stages second-order phase shifter allpass filters. 

    Phaser implements `num` number of second-order allpass filters.

    Parent class : PyoObject

    Parameters:

    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Center frequency of the first notch. Defaults to 1000.
    spread : float or PyoObject, optional
        Spreading factor for upper notch frequencies. Defaults to 1.1.
    q : float or PyoObject, optional
        Q of the filter as center frequency / bandwidth. Defaults to 10.
    feedback : float or PyoObject, optional
        Amount of output signal which is fed back into the input of the
        allpass chain. Defaults to 0.
    num : int, optional
        The number of allpass stages in series. Determine the number of
        notches in the spectrum. Available at initialization only.
        Defaults to 8.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFreq(x) : Replace the `freq` attribute.
    setSpread(x) : Replace the `spread` attribute.
    setQ(x) : Replace the `q` attribute.
    setFeedback(x) : Replace the `feedback` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    freq : float or PyoObject. Center frequency of the first notch.
    spread : float or PyoObject. Spreading factor for upper notch frequencies.
    q : float or PyoObject. Q factor of the filter.
    feedback : float or PyoObject. Amount of output signal fed back in input.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> fade = Fader(fadein=.1, mul=.1).play()
    >>> a = Noise(fade).out()
    >>> lf1 = Sine(freq=.1, mul=100, add=250)
    >>> lf2 = Sine(freq=.15, mul=.4, add=1.5)
    >>> b = Phaser(a, freq=lf1, spread=lf2, q=1, num=20, mul=.5).out(1)

    """
    def __init__(self, input, freq=1000, spread=1.1, q=10, feedback=0, num=8, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._spread = spread
        self._q = q
        self._feedback = feedback
        self._num= num
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, spread, q, feedback, num, mul, add, lmax = convertArgsToLists(self._in_fader, freq, spread, q, feedback, num, mul, add)
        self._base_objs = [Phaser_base(wrap(in_fader,i), wrap(freq,i), wrap(spread,i), wrap(q,i), wrap(feedback,i), wrap(num,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'freq', 'spread', 'q', 'feedback', 'mul', 'add']

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

    def setSpread(self, x):
        """
        Replace the `spread` attribute.

        Parameters:

        x : float or PyoObject
            New `spread` attribute.

        """
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute.

        Parameters:

        x : float or PyoObject
            New `q` attribute.

        """
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(20, 2000, "log", "freq", self._freq), 
                          SLMap(0.5, 2, "lin", "spread", self._spread),
                          SLMap(0.5, 100, "log", "q", self._q), 
                          SLMap(0, 1, "lin", "feedback", self._feedback),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the first notch.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def spread(self):
        """float or PyoObject. Spreading factor for upper notch frequencies.""" 
        return self._spread
    @spread.setter
    def spread(self, x): self.setSpread(x)

    @property
    def q(self):
        """float or PyoObject. Q factor of the filter.""" 
        return self._q
    @q.setter
    def q(self, x): self.setQ(x)

    @property
    def feedback(self):
        """float or PyoObject. Feedback factor of the filter.""" 
        return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)
