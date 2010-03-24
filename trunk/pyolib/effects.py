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
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
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

    def __dir__(self):
        return ['input', 'drive', 'slope', 'mul', 'add']

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
        self._map_list = [SLMap(0., 1., 'lin', 'drive', self._drive),
                          SLMap(0., 0.999, 'lin', 'slope', self._slope),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
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

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0.001, self._maxdelay, 'log', 'delay',  self._delay),
                          SLMap(0., 1., 'lin', 'feedback', self._feedback),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    Basic waveguide model.
    
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

    def __dir__(self):
        return ['input', 'freq', 'dur', 'mul', 'add']
        
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
        self._map_list = [SLMap(10, 500., 'log', 'freq',  self._freq),
                          SLMapDur(self._dur),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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
    Implementation of Jezar's Freeverb.
    
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
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
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

    def __dir__(self):
        return ['input', 'size', 'damp', 'bal', 'mul', 'add']

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
        self._map_list = [SLMap(0., 1., 'lin', 'size',  self._size),
                          SLMap(0., 1., 'lin', 'damp',  self._damp),
                          SLMap(0., 1., 'lin', 'bal',  self._bal),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

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

class Convolve(PyoObject):
    """
    Implements filtering using circular convolution.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    table : PyoTableObject
        Table containning the impulse response.
    size : int
        Length, in samples, of the convolution. Available at initialization 
        time only. If the table changes during the performance, its size
        must egal or greater than this value. If greater only the first
        `size` samples will be used.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setTable(x) : Replace the `table` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    table : PyoTableObject. Table containning the impulse response.
    
    Notes :
    
    Convolution is very expensive to compute, so the impulse response must
    be kept very short to run in real time.
    
    Usually convolution generates a high amplitude level, take care of the
    `mul` parameter!
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SNDS_PATH + '/transparent.aif'
    >>> sf = SfPlayer(snd, loop=True, mul=.5).out()
    >>> a = Convolve(sf, '/Users/olipet/impulse3_512.aif').out()

    """
    def __init__(self, input, table, size, mul=1, add=0):
        self._input = input
        self._table = table
        self._size = size
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, table, size, mul, add, lmax = convertArgsToLists(self._in_fader, table, size, mul, add)                     
        self._base_objs = [Convolve_base(wrap(in_fader,i), wrap(table,i), wrap(size,i), wrap(mul,i), wrap(add,i)) \
                               for i in range(lmax)]

    def __dir__(self):
        return ['input', 'table', 'mul', 'add']
        
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

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
      
    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def table(self):
        """PyoTableObject. Table containing the impulse response.""" 
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

class WGVerb(PyoObject):
    """
    8 delay line mono FDN reverb.
    
    8 delay line FDN reverb, with feedback matrix based upon physical 
    modeling scattering junction of 8 lossless waveguides of equal 
    characteristic impedance.
    
    Parent class : PyoObject

    Parameters:
    
    input : PyoObject
        Input signal to reverberated.
    feedback : float or PyoObject, optional
        Amount of output signal sent back into the delay lines.
        0.6 gives a good small "live" room sound, 0.8 a small hall, 
        and 0.9 a large hall. Defaults to 0.5.
    cutoff : float or PyoObject, optional
        cutoff frequency of simple first order lowpass filters in the 
        feedback loop of delay lines, in Hz. Defaults to 5000.
    mix : float, optional
        Balance between wet and dry signal, between 0 and 1. 0 means no 
        reverb. Defaults to 0.5.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFeedback(x) : Replace the `feedback` attribute.
    setCutoff(x) : Replace the `cutoff` attribute.
    setMix(x) : Replace the `mix` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to delayed.
    feedback : float or PyoObject. Amount of output signal sent back 
        into the delay line.
    cutoff : float or PyoObject. Internal lowpass filter cutoff 
        frequency in Hz.
    mix : float or PyoObject. Balance between wet and dry signal.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> d = WGVerb(a, feedback=.75, cutoff=5000, mix=.25, mul=.5).out()

    """
    def __init__(self, input, feedback=0.5, cutoff=5000, mix=0.5, mul=1, add=0):
        self._input = input
        self._feedback = feedback
        self._cutoff = cutoff
        self._mix = mix
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, feedback, cutoff, mix, mul, add, lmax = convertArgsToLists(self._in_fader, feedback, cutoff, mix, mul, add)
        self._base_objs = [WGVerb_base(wrap(in_fader,i), wrap(feedback,i), wrap(cutoff,i), wrap(mix,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'feedback', 'cutoff', 'mix', 'mul', 'add']
        
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

    def setCutoff(self, x):
        """
        Replace the `cutoff` attribute.
        
        Parameters:

        x : float or PyoObject
            New `cutoff` attribute.

        """
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMix(self, x):
        """
        Replace the `mix` attribute.
        
        Parameters:

        x : float or PyoObject
            New `mix` attribute.

        """
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setMix(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMap(0., 1., 'lin', 'feedback', self._feedback),
                          SLMap(500., 15000., 'log', 'cutoff', self._cutoff),
                          SLMap(0., 1., 'lin', 'mix', self._mix),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

    @property
    def input(self):
        """PyoObject. Input signal to delayed.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def feedback(self):
        """float or PyoObject. Amount of output signal sent back into the delay line.""" 
        return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)

    @property
    def cutoff(self):
        """float or PyoObject. Lowpass filter cutoff in Hz.""" 
        return self._cutoff
    @cutoff.setter
    def cutoff(self, x): self.setCutoff(x)

    @property
    def mix(self):
        """float or PyoObject. wet - dry balance.""" 
        return self._mix
    @mix.setter
    def mix(self, x): self.setMix(x)
