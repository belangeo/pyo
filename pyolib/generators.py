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

    def __dir__(self):
        return ['freq', 'phase', 'mul', 'add']
        
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
        self._map_list = [SLMapFreq(self._freq), SLMapPhase(self._phase), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)
        
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

class SineLoop(PyoObject):
    """
    A simple sine wave oscillator with feedback.
    
    The oscillator output, multiplied by `feedback`, is added to the position
    increment and can be used to control the brightness of the oscillator.

    Parent class: PyoObject

    Parameters:

    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1000.
    feedback : float or PyoObject, optional
        Amount of the output signal added to position increment, between 0 and 1. 
        Controls the brightness. Defaults to 0.

    Methods:

    setFreq(x) : Replace the `freq` attribute.
    setFeedback(x) : Replace the `feedback` attribute.

    Attributes:

    freq : float or PyoObject, Frequency in cycles per second.
    feedback : float or PyoObject, Brightness control.

    See also: Sine, OscLoop

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(.1, 0, .1, .1)
    >>> a = SineLoop(freq=400, feedback=lfo).out()

    """
    def __init__(self, freq=1000, feedback=0, mul=1, add=0):
        self._freq = freq
        self._feedback = feedback
        self._mul = mul
        self._add = add
        freq, feedback, mul, add, lmax = convertArgsToLists(freq, feedback, mul, add)
        self._base_objs = [SineLoop_base(wrap(freq,i), wrap(feedback,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['freq', 'feedback', 'mul', 'add']

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
        self._map_list = [SLMapFreq(self._freq), SLMap(0, 1, "lin", "feedback", self._feedback), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

    def __dir__(self):
        return ['freq', 'phase', 'mul', 'add']

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
        self._map_list = [SLMapFreq(self._freq), SLMapPhase(self._phase), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)
        
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

    def __dir__(self):
        return ['mul', 'add']

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

    def __dir__(self):
        return ['mul', 'add']

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

class FM(PyoObject):
    """
    A simple frequency modulation generator.
    
    Implements frequency modulation synthesis based on Chowning's algorithm.
    
    Parent class: PyoObject
    
    Parameters:
    
    carrier : float or PyoObject, optional
        Carrier frequency in cycles per second. Defaults to 100.
    ratio : float or PyoObject, optional
        A factor that, when multiplied by the `carrier` parameter, 
        gives the modulator frequency. Defaults to 0.5.
    index : float or PyoObject, optional
        The modulation index. This value multiplied by the modulator
        frequency gives the modulator amplitude. Defaults to 5.
        
    Methods:
    
    setCarrier(x) : Replace the `carrier` attribute.
    setRatio(x) : Replace the `ratio` attribute.
    setIndex(x) : Replace the `index` attribute.
    
    Attributes:
    
    carrier : float or PyoObject, Carrier frequency in cycles per second.
    ratio : float or PyoObject, Modulator/Carrier ratio.
    index : float or PyoObject, Modulation index.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> ind = LinTable([(0,20), (200,5), (1000,2), (8191,1)])
    >>> m = Metro(4).play()
    >>> tr = TrigEnv(m, ind, dur=4)
    >>> f = FM(carrier=[250.5,250], ratio=.2499, index=tr, mul=.5).out()
    
    """
    def __init__(self, carrier=100, ratio=0.5, index=5, mul=1, add=0):
        self._carrier = carrier
        self._ratio = ratio
        self._index = index
        self._mul = mul
        self._add = add
        carrier, ratio, index, mul, add, lmax = convertArgsToLists(carrier, ratio, index, mul, add)
        self._base_objs = [Fm_base(wrap(carrier,i), wrap(ratio,i), wrap(index,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['carrier', 'ratio', 'index', 'mul', 'add']
        
    def setCarrier(self, x):
        """
        Replace the `carrier` attribute.
        
        Parameters:

        x : float or PyoObject
            new `carrier` attribute.
        
        """
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setRatio(self, x):
        """
        Replace the `ratio` attribute.
        
        Parameters:

        x : float or PyoObject
            new `ratio` attribute.
        
        """
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.
        
        Parameters:

        x : float or PyoObject
            new `index` attribute.
        
        """
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(10, 500, "lin", "carrier", self._carrier),
                          SLMap(.01, 10, "lin", "ratio", self._ratio),
                          SLMap(0, 20, "lin", "index", self._index),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)
        
    @property
    def carrier(self):
        """float or PyoObject. Carrier frequency in cycles per second.""" 
        return self._carrier
    @carrier.setter
    def carrier(self, x): self.setCarrier(x)

    @property
    def ratio(self):
        """float or PyoObject. Modulator/Carrier ratio.""" 
        return self._ratio
    @ratio.setter
    def ratio(self, x): self.setRatio(x)

    @property
    def index(self):
        """float or PyoObject. Modulation index.""" 
        return self._index
    @index.setter
    def index(self, x): self.setIndex(x)
