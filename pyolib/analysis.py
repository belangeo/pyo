"""
Tools to analyze audio signals.

These objects are designed to retrieve specific informations
from an audio stream. Analysis are sent at audio rate, user 
can use them for controlling parameters of others objects.

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

    :Parent: :py:class:`PyoObject`

    :Args:
    
        input : PyoObject
            Input signal to process.
        freq : float or PyoObject, optional
            Cutoff frequency of the filter in hertz. Default to 20.

    .. note::

        The out() method is bypassed. Follower's signal can not be sent to 
        audio outs.
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> fol = Follower(sf, freq=30)
    >>> n = Noise(mul=fol).out(1)

    """
    def __init__(self, input, freq=20, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [Follower_base(wrap(in_fader,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        :Args:

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
        
        :Args:

            x : float or PyoObject
                New `freq` attribute.

        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(1., 500., 'log', 'freq', self._freq)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. Input signal to process."""
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

    :Parent: :py:class:`PyoObject`

    :Args:

        input : PyoObject
            Input signal to process.
        risetime : float or PyoObject, optional
            Time to reach upward value in seconds. Default to 0.01.
        falltime : float or PyoObject, optional
            Time to reach downward value in seconds. Default to 0.1.

    .. note::

        The out() method is bypassed. Follower's signal can not be sent to 
        audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> fol2 = Follower2(sf, risetime=0.002, falltime=.1, mul=.5)
    >>> n = Noise(fol2).out(1)

    """
    def __init__(self, input, risetime=0.01, falltime=0.1, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, mul, add)
        self._base_objs = [Follower2_base(wrap(in_fader,i), wrap(risetime,i), wrap(falltime, i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

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

        :Args:

            x : float or PyoObject
                New `risetime` attribute.

        """
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRisetime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFalltime(self, x):
        """
        Replace the `falltime` attribute.

        :Args:

            x : float or PyoObject
                New `falltime` attribute.

        """
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFalltime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.001, 1., 'log', 'risetime', self._risetime)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
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
 
    :Parent: :py:class:`PyoObject`
   
    :Args:
    
        input : PyoObject
            Input signal to process.
        thresh : float, optional
            Minimum amplitude difference allowed between adjacent samples 
            to be included in the zeros count.

    .. note::

        The out() method is bypassed. ZCross's signal can not be sent to 
        audio outs.
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> b = ZCross(a, thresh=.02)
    >>> n = Noise(b).out(1)

    """
    def __init__(self, input, thresh=0., mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._thresh = thresh
        self._in_fader = InputFader(input)
        in_fader, thresh, mul, add, lmax = convertArgsToLists(self._in_fader, thresh, mul, add)
        self._base_objs = [ZCross_base(wrap(in_fader,i), wrap(thresh,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        :Args:

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
        
        :Args:

            x : float
                New amplitude difference threshold.

        """
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 0.5, 'lin', 'thresh', self._thresh)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
     
    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def thresh(self):
        """float. Amplitude difference threshold.""" 
        return self._thresh
    @thresh.setter
    def thresh(self, x): self.setThresh(x)

class Yin(PyoObject):
    """
    Pitch tracker using the Yin algorithm.
    
    Pitch tracker using the Yin algorithm based on the implementation in C of aubio.
    This algorithm was developped by A. de Cheveigne and H. Kawahara and published in
    
    de Cheveigne, A., Kawahara, H. (2002) 'YIN, a fundamental frequency estimator for
    speech and music', J. Acoust. Soc. Am. 111, 1917-1930.
    
    The audio output of the object is the estimated frequency, in Hz, of the input sound.
 
    :Parent: :py:class:`PyoObject`
   
    :Args:
    
        input : PyoObject
            Input signal to process.
        tolerance : float, optional
            Parameter for minima selection, between 0 and 1. Defaults to 0.2.
        minfreq : float, optional
            Minimum estimated frequency in Hz. Frequency below this threshold will 
            be ignored. Defaults to 40.
        maxfreq : float, optional
            Maximum estimated frequency in Hz. Frequency above this threshold will 
            be ignored. Defaults to 1000.
        cutoff : float, optional
            Cutoff frequency, in Hz, of the lowpass filter applied on the input sound.
            Defaults to 1000.
            
            The lowpass filter helps the algorithm to detect the fundamental frequency by filtering
            higher harmonics. 
        winsize : int, optional
            Size, in samples, of the analysis window. Must be higher that two period
            of the lowest desired frequency.
            
            Available at initialization time only.  Defaults to 1024.
            

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> lfo = Randh(min=100, max=500, freq=3)
    >>> src = SineLoop(freq=lfo, feedback=0.1, mul=.3).out()
    >>> pit = Yin(src, tolerance=0.2, winsize=1024)
    >>> freq = Tone(pit, freq=10)
    >>> # fifth above
    >>> a = LFO(freq*1.5, type=2, mul=0.2).out(1)

    """
    def __init__(self, input, tolerance=0.2, minfreq=40, maxfreq=1000, cutoff=1000, winsize=1024, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._tolerance = tolerance
        self._minfreq = minfreq
        self._maxfreq = maxfreq
        self._cutoff = cutoff
        self._in_fader = InputFader(input)
        in_fader, tolerance, minfreq, maxfreq, cutoff, winsize, mul, add, lmax = convertArgsToLists(self._in_fader, tolerance, minfreq, maxfreq, cutoff, winsize, mul, add)
        self._base_objs = [Yin_base(wrap(in_fader,i), wrap(tolerance,i), wrap(minfreq,i), wrap(maxfreq,i), wrap(cutoff,i), wrap(winsize,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        :Args:

            x : PyoObject
                New signal to process.
            fadetime : float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTolerance(self, x):
        """
        Replace the `tolerance` attribute.
        
        :Args:

            x : float
                New parameter for minima selection, between 0 and 1.

        """
        self._tolerance = x
        x, lmax = convertArgsToLists(x)
        [obj.setTolerance(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMinfreq(self, x):
        """
        Replace the `minfreq` attribute.
        
        :Args:

            x : float
                New minimum frequency detected.

        """
        self._minfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinfreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMaxfreq(self, x):
        """
        Replace the `maxfreq` attribute.
        
        :Args:

            x : float
                New maximum frequency detected.

        """
        self._maxfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxfreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        """
        Replace the `cutoff` attribute.
        
        :Args: 

            x : float
                New input lowpass filter cutoff frequency.

        """
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, 1, 'lin', 'tolerance', self._tolerance, dataOnly=True),
                          SLMap(20, 400, 'log', 'minfreq', self._minfreq, dataOnly=True),
                          SLMap(500, 5000, 'log', 'maxfreq', self._maxfreq, dataOnly=True),
                          SLMap(200, 15000, 'log', 'cutoff', self._cutoff, dataOnly=True)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
     
    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def tolerance(self):
        """float. Parameter for minima selection.""" 
        return self._tolerance
    @tolerance.setter
    def tolerance(self, x): self.setTolerance(x)

    @property
    def minfreq(self):
        """float. Minimum frequency detected.""" 
        return self._minfreq
    @minfreq.setter
    def minfreq(self, x): self.setMinfreq(x)

    @property
    def maxfreq(self):
        """float. Maximum frequency detected.""" 
        return self._maxfreq
    @maxfreq.setter
    def maxfreq(self, x): self.setMaxfreq(x)

    @property
    def cutoff(self):
        """float. Input lowpass filter cutoff frequency.""" 
        return self._cutoff
    @cutoff.setter
    def cutoff(self, x): self.setCutoff(x)

class Centroid(PyoObject):
    """
    Computes the spectral centroid of an input signal.
    
    Output signal is the spectral centroid, in Hz, of the input signal.
    It indicates where the "center of mass" of the spectrum is. Perceptually, 
    it has a robust connection with the impression of "brightness" of a sound.
    
    Centroid does its computation with two overlaps, so a new output value 
    comes every half of the FFT window size.
 
    :Parent: :py:class:`PyoObject`
   
    :Args:
    
        input : PyoObject
            Input signal to process.
        size : int, optional
            Size, as a power-of-two, of the FFT used to compute the centroid.

            Available at initialization time only.  Defaults to 1024.
            

    .. note::

        The out() method is bypassed. Centroid's signal can not be sent to 
        audio outs.
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> b = Centroid(a, 1024)
    >>> c = Port(b, 0.05, 0.05)
    >>> d = ButBP(Noise(0.2), freq=c, q=5).out(1)

    """
    def __init__(self, input, size=1024, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._size = size
        self._in_fader = InputFader(input)
        in_fader, size, mul, add, lmax = convertArgsToLists(self._in_fader, size, mul, add)
        self._base_objs = [Centroid_base(wrap(in_fader,i), wrap(size,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        :Args:

            x : PyoObject
                New signal to process.
            fadetime : float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
     
    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class AttackDetector(PyoObject):
    """
    Audio signal onset detection.
    
    AttackDetector anaylises an audio signal in input an output a trigger each
    time an onset is detected. An onset is a sharp amplitude rising while the
    signal had previously fall below a minimum threshold. Parameters must be
    carefully tuned depending on the nature of the analysed signal and the level
    of the background noise. 

    :Parent: :py:class:`PyoObject`
   
    :Args:
    
        input : PyoObject
            Input signal to process.
        deltime : float, optional
            Delay time, in seconds, between previous and current rms analysis to compare.
            Defaults to 0.005.
        cutoff : float, optional
            Cutoff frequency, in Hz, of the amplitude follower's lowpass filter. 
            Defaults to 10.
            
            Higher values are more responsive and also more likely to give false onsets. 
        maxthresh : float, optional
            Attack threshold in positive dB (current rms must be higher than previous 
            rms + maxthresh to be reported as an attack). Defaults to 3.0.
        minthresh : float, optional
            Minimum threshold in dB (signal must fall below this threshold to allow 
            a new attack to be detected). Defaults to -30.0.
        reltime : float, optional
            Time, in seconds, to wait before reporting a new attack. Defaults to 0.1.
            

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input()
    >>> d = AttackDetector(a, deltime=0.005, cutoff=10, maxthresh=4, minthresh=-20, reltime=0.05)
    >>> exc = TrigEnv(d, HannTable(), dur=0.005, mul=BrownNoise(0.3))
    >>> wgs = Waveguide(exc, freq=[100,200.1,300.3,400.5], dur=30).out()

    """
    def __init__(self, input, deltime=0.005, cutoff=10, maxthresh=3, minthresh=-30, reltime=0.1, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._deltime = deltime
        self._cutoff = cutoff
        self._maxthresh = maxthresh
        self._minthresh = minthresh
        self._reltime = reltime
        self._in_fader = InputFader(input)
        in_fader, deltime, cutoff, maxthresh, minthresh, reltime, mul, add, lmax = convertArgsToLists(self._in_fader, deltime, cutoff, maxthresh, minthresh, reltime, mul, add)
        self._base_objs = [AttackDetector_base(wrap(in_fader,i), wrap(deltime,i), wrap(cutoff,i), wrap(maxthresh,i), wrap(minthresh,i), wrap(reltime,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        :Args:

            x : PyoObject
                New signal to process.
            fadetime : float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDeltime(self, x):
        """
        Replace the `deltime` attribute.
        
        :Args:

            x : float
                New delay between rms analysis.

        """
        self._deltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setDeltime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        """
        Replace the `cutoff` attribute.
        
        :Args:

            x : float
                New cutoff for the follower lowpass filter.

        """
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMaxthresh(self, x):
        """
        Replace the `maxthresh` attribute.
        
        :Args:

            x : float
                New attack threshold in dB.

        """
        self._maxthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxthresh(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMinthresh(self, x):
        """
        Replace the `minthresh` attribute.
        
        :Args:

            x : float
                New minimum threshold in dB.

        """
        self._minthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinthresh(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setReltime(self, x):
        """
        Replace the `reltime` attribute.
        
        :Args: 

            x : float
                Time, in seconds, to wait before reporting a new attack.

        """
        self._reltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setReltime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.001, 0.05, 'lin', 'deltime', self._deltime, dataOnly=True),
                          SLMap(1.0, 1000.0, 'log', 'cutoff', self._cutoff, dataOnly=True),
                          SLMap(0.0, 18.0, 'lin', 'maxthresh', self._maxthresh, dataOnly=True),
                          SLMap(-90.0, 0.0, 'lin', 'minthresh', self._minthresh, dataOnly=True),
                          SLMap(0.001, 1.0, 'log', 'reltime', self._reltime, dataOnly=True)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
     
    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def deltime(self):
        """float. Delay between rms analysis.""" 
        return self._deltime
    @deltime.setter
    def deltime(self, x): self.setDeltime(x)

    @property
    def cutoff(self):
        """float. Cutoff for the follower lowpass filter.""" 
        return self._cutoff
    @cutoff.setter
    def cutoff(self, x): self.setCutoff(x)

    @property
    def maxthresh(self):
        """float. Attack threshold in dB.""" 
        return self._maxthresh
    @maxthresh.setter
    def maxthresh(self, x): self.setMaxthresh(x)

    @property
    def minthresh(self):
        """float. Minimum threshold in dB.""" 
        return self._minthresh
    @minthresh.setter
    def minthresh(self, x): self.setMinthresh(x)

    @property
    def reltime(self):
        """float. Time to wait before reporting a new attack.""" 
        return self._reltime
    @reltime.setter
    def reltime(self, x): self.setReltime(x)
