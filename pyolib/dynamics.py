"""
Objects to modify the dynamic range and sample quality of audio streams.

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
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> lfoup = Sine(freq=.25, mul=.48, add=.5)
    >>> lfodown = 0 - lfoup
    >>> c = Clip(a, min=lfodown, max=lfoup, mul=.5).out()

    """
    def __init__(self, input, min=-1.0, max=1.0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._min = min
        self._max = max
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [Clip_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'min', 'max', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(-1., 0., 'lin', 'min', self._min),
                          SLMap(0., 1., 'lin', 'max', self._max),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
        """float or PyoObject. Maximum possible value.""" 
        return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class Mirror(PyoObject):
    """
    Reflects the signal that exceeds the min and max thresholds.

    This object is useful for table indexing or for clipping and
    modeling an audio signal.

    Parent class : PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.
    min : float or PyoObject, optional
        Minimum possible value. Defaults to 0.
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

    Notes:
    
    If `min` is higher than `max`, then the output will be the average of the two.
    
    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(freq=[399.1,397.8,404.3,397.5,399.5,402.9,396.1,396.3])
    >>> lfmin = Sine(freq=1.5, mul=.25, add=-0.75)
    >>> lfmax = Sine(freq=2, mul=.25, add=0.75)
    >>> b = Mirror(a, min=lfmin, max=lfmax)
    >>> c = Tone(b, freq=500, mul=.1).out()

    """
    def __init__(self, input, min=0.0, max=1.0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._min = min
        self._max = max
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [Mirror_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'min', 'max', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(0., 1., 'lin', 'max', self._max),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
        """float or PyoObject. Maximum possible value.""" 
        return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class Wrap(PyoObject):
    """
    Wraps-around the signal that exceeds the min and max thresholds.

    This object is useful for table indexing, phase shifting or for 
    clipping and modeling an audio signal.

    Parent class : PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.
    min : float or PyoObject, optional
        Minimum possible value. Defaults to 0.
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

    Notes:

    If `min` is higher than `max`, then the output will be the average of the two.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> env = HannTable()
    >>> lff = Sine(.5, mul=3, add=4)
    >>> ph1 = Phasor(lff)
    >>> ph2 = Wrap(ph1+0.5, min=0, max=1)
    >>> amp1 = Pointer(env, ph1, mul=.3)
    >>> amp2 = Pointer(env, ph2, mul=.3)
    >>> a = SineLoop(300, feedback=.1, mul=amp1).out()
    >>> b = SineLoop(350, feedback=.1, mul=amp2).out(1)

    """
    def __init__(self, input, min=0.0, max=1.0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._min = min
        self._max = max
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [Wrap_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'min', 'max', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(0., 1., 'lin', 'max', self._max),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
        """float or PyoObject. Maximum possible value.""" 
        return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class Degrade(PyoObject):
    """
    Signal quality reducer.
    
    Degrade takes an audio signal and reduces the sampling rate and/or 
    bit-depth as specified.
    
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to process.
    bitdepth : float or PyoObject, optional
        Signal quantization in bits. Must be in range 1 -> 32.
        Defaults to 16.
    srscale : float or PyoObject, optional
        Sampling rate multiplier. Must be in range 0.0009765625 -> 1.
        Defaults to 1.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setBitdepth(x) : Replace the `bitdepth` attribute.
    setSrscale(x) : Replace the `srscale` attribute.

    Attributes:
    
    input : PyoObject. Input signal to filter.
    bitdepth : float or PyoObject. Quantization in bits.
    srscale : float or PyoObject. Sampling rate multiplier.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SquareTable()
    >>> a = Osc(table=t, freq=100)
    >>> lfo = Sine(freq=.2, mul=6, add=8)
    >>> lfo2 = Sine(freq=.25, mul=.45, add=.55)
    >>> b = Degrade(a, bitdepth=lfo, srscale=lfo2).out()
    
    """
    def __init__(self, input, bitdepth=16, srscale=1.0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._bitdepth = bitdepth
        self._srscale = srscale
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, bitdepth, srscale, mul, add, lmax = convertArgsToLists(self._in_fader, bitdepth, srscale, mul, add)
        self._base_objs = [Degrade_base(wrap(in_fader,i), wrap(bitdepth,i), wrap(srscale,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'bitdepth', 'srscale', 'mul', 'add']

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
 
    def setBitdepth(self, x):
        """
        Replace the `bitdepth` attribute.
        
        Parameters:

        x : float or PyoObject
            New `bitdepth` attribute.

        """
        self._bitdepth = x
        x, lmax = convertArgsToLists(x)
        [obj.setBitdepth(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSrscale(self, x):
        """
        Replace the `srscale` attribute.
        
        Parameters:

        x : float or PyoObject
            New `srscale` attribute.

        """
        self._srscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setSrscale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(1., 32., 'log', 'bitdepth', self._bitdepth),
                          SLMap(0.0009765625, 1., 'log', 'srscale', self._srscale),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def bitdepth(self):
        """float or PyoObject. Signal quantization in bits.""" 
        return self._bitdepth
    @bitdepth.setter
    def bitdepth(self, x): self.setBitdepth(x)

    @property
    def srscale(self):
        """float or PyoObject. Sampling rate multiplier.""" 
        return self._srscale
    @srscale.setter
    def srscale(self, x): self.setSrscale(x)

class Compress(PyoObject):
    """
    Reduces the dynamic range of an audio signal.
    
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
    >>> a = SfPlayer(SNDS_PATH + '/transparent.aif', loop=True)
    >>> b = Compress(a, thresh=-30, ratio=4, risetime=.005, falltime=.1).out()
    
    """
    def __init__(self, input, thresh=-20, ratio=2, risetime=0.005, falltime=0.05, mul=1, add=0):
        PyoObject.__init__(self)
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

    def __dir__(self):
        return ['input', 'thresh', 'ratio', 'risetime', 'falltime', 'mul', 'add']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(-90., 0., 'lin', 'thresh',  self._thresh),
                          SLMap(1., 10., 'lin', 'ratio',  self._ratio),
                          SLMap(0.001, .2, 'lin', 'risetime',  self._risetime),
                          SLMap(0.001, .2, 'lin', 'falltime',  self._falltime),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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