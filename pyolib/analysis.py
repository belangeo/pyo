"""
Tools to analyze audio signals.

These objects are designed to retrieve specific informations from
an audio stream. Analysis are sent at audio rate, user can 
use them for controlling parameters of others objects.

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

class Follower(PyoObject):
    """
    Envelope follower.
    
    Output signal is the continuous mean amplitude of an input signal.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    freq : float or PyoObject, optional
        Cutoff frequency of the filter in hertz. Default to 20.

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
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.5).out()
    >>> fol = Follower(sf, freq=30)
    >>> n = Noise(mul=fol).out(1)

    """
    def __init__(self, input, freq=20, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._freq = freq
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [Follower_base(wrap(in_fader,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(1., 500., 'log', 'freq', self._freq)]
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

class Follower2(PyoObject):
    """
    Envelope follower with different attack and release times.

    Output signal is the continuous mean amplitude of an input signal.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to filter.
    risetime : float or PyoObject, optional
        Time to reach upward value in seconds. Default to 0.01.
    falltime : float or PyoObject, optional
        Time to reach downward value in seconds. Default to 0.1.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setRisetime(x) : Replace the `risetime` attribute.
    setFalltime(x) : Replace the `falltime` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    risetime : float or PyoObject. Time to reach upward value in seconds.
    falltime : float or PyoObject. Time to reach downward value in seconds.

    Notes:

    The out() method is bypassed. Follower's signal can not be sent to 
    audio outs.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.5).out()
    >>> fol2 = Follower2(sf, risetime=0.002, falltime=.1, mul=.5)
    >>> n = Noise(fol2).out(1)

    """
    def __init__(self, input, risetime=0.01, falltime=0.1, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, mul, add)
        self._base_objs = [Follower2_base(wrap(in_fader,i), wrap(risetime,i), wrap(falltime, i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'risetime', 'falltime', 'mul', 'add']

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

    def setRisetime(self, x):
        """
        Replace the `risetime` attribute.

        Parameters:

        x : float or PyoObject
            New `risetime` attribute.

        """
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRisetime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFalltime(self, x):
        """
        Replace the `falltime` attribute.

        Parameters:

        x : float or PyoObject
            New `falltime` attribute.

        """
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFalltime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.001, 1., 'log', 'risetime', self._risetime)]
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
    def risetime(self, x): self.setRisetime(x)

    @property
    def falltime(self):
        """float or PyoObject. Time to reach downward value in seconds.""" 
        return self._falltime
    @falltime.setter
    def falltime(self, x): self.setFalltime(x)

class ZCross(PyoObject):
    """
    Zero-crossing counter.
    
    Output signal is the number of zero-crossing occured during each 
    buffer size, normalized between 0 and 1.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    thresh : float, optional
        Minimum amplitude difference allowed between adjacent samples 
        to be included in the zeros count.
         

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setThresh(x) : Replace the `thresh` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    thresh : float. Amplitude difference threshold.

    Notes:

    The out() method is bypassed. ZCross's signal can not be sent to 
    audio outs.
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input()
    >>> b = ZCross(a, thresh=.02)
    >>> n = Noise(b).out()

    """
    def __init__(self, input, thresh=0., mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._thresh = thresh
        self._in_fader = InputFader(input)
        in_fader, thresh, mul, add, lmax = convertArgsToLists(self._in_fader, thresh, mul, add)
        self._base_objs = [ZCross_base(wrap(in_fader,i), wrap(thresh,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'thresh', 'mul', 'add']

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

    def setThresh(self, x):
        """
        Replace the `thresh` attribute.
        
        Parameters:

        x : float
            New amplitude difference threshold.

        """
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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

    @property
    def thresh(self):
        """float. Amplitude difference threshold.""" 
        return self._thresh
    @thresh.setter
    def thresh(self, x): self.setThresh(x)

