"""
Objects designed to create parameter's control at audio rate. 

They create envelopes, line segments and conversion from number to 
audio signal. 

The audio streams of these objects can't be sent to the output 
soundcard.
 
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

######################################################################
### Controls
######################################################################                                       
class Fader(PyoObject):
    """
    Fadein - fadeout envelope generator.
    
    Generate an amplitude envelope between 0 and 1 with control on fade 
    times and total duration of the envelope.
    
    The play() method starts the envelope and is not called at the 
    object creation time.
    
    Parent class: PyoObject

    Parameters:

    fadein : float, optional
        Rising time of the envelope in seconds. Defaults to 0.01.
    fadeout : float, optional
        Falling time of the envelope in seconds. Defaults to 0.1.
    dur : float, optional
        Total duration of the envelope. Defaults to 0, which means wait 
        for the stop() method to start the fadeout.
        
    Methods:

    play() : Start processing without sending samples to the output. 
        Triggers the envelope.
    stop() : Stop processing. Triggers the envelope's fadeout 
        if `dur` is set to 0.
    setFadein(x) : Replace the `fadein` attribute.
    setFadeout(x) : Replace the `fadeout` attribute.
    setDur(x) : Replace the `dur` attribute.

    Attributes:
    
    fadein : float. Rising time of the envelope in seconds.
    fadeout : float. Falling time of the envelope in seconds.
    dur : float. Total duration of the envelope.
    
    Notes:

    The out() method is bypassed. Fader's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> f = Fader(fadein=1, fadeout=2, dur=5, mul=.5)
    >>> a = Noise(mul=f).out()
    >>> f.play()    
    
    """
    def __init__(self, fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._fadein = fadein
        self._fadeout = fadeout
        self._dur = dur
        self._mul = mul
        self._add = add
        fadein, fadeout, dur, mul, add, lmax = convertArgsToLists(fadein, fadeout, dur, mul, add)
        self._base_objs = [Fader_base(wrap(fadein,i), wrap(fadeout,i), wrap(dur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['fadein', 'fadeout', 'dur', 'mul', 'add']

    def out(self, chnl=0, inc=1):
        """
        Bypassed. Can't be sent to audio outs.
        """
        pass

    def setFadein(self, x):
        """
        Replace the `fadein` attribute.
        
        Parameters:

        x : float
            new `fadein` attribute.
        
        """
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        """
        Replace the `fadeout` attribute.
        
        Parameters:

        x : float
            new `fadeout` attribute.
        
        """
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.
        
        Parameters:

        x : float
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

    @property
    def fadein(self):
        """float. Rising time of the envelope in seconds.""" 
        return self._fadein
    @fadein.setter
    def fadein(self, x): self.setFadein(x)

    @property
    def fadeout(self):
        """float. Falling time of the envelope in seconds.""" 
        return self._fadeout
    @fadeout.setter
    def fadeout(self, x): self.setFadeout(x)

    @property
    def dur(self):
        """float. Total duration of the envelope.""" 
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)


class Adsr(PyoObject):
    """
    Attack - Decay - Sustain - Release envelope generator.
    
    Calculates the classical ADSR envelope using linear segments. 
    Duration can be set to 0 to give an infinite sustain. In this 
    case, the stop() method calls the envelope release part.
     
    The play() method starts the envelope and is not called at the 
    object creation time.
    
    Parent class: PyoObject

    Parameters:

    attack : float, optional
        Duration of the attack phase in seconds. Defaults to 0.01.
    decay : float, optional
        Duration of the decay in seconds. Defaults to 0.05.
    sustain : float, optional
        Amplitude of the sustain phase. Defaults to 0.707.
    release : float, optional
        Duration of the release in seconds. Defaults to 0.1.
    dur : float, optional
        Total duration of the envelope. Defaults to 0, which means wait 
        for the stop() method to start the release phase.
        
    Methods:

    play() : Start processing without sending samples to the output. 
        Triggers the envelope.
    stop() : Stop processing. Triggers the envelope's fadeout 
        if `dur` is set to 0.
    setAttack(x) : Replace the `attack` attribute.
    setDecay(x) : Replace the `decay` attribute.
    setSustain(x) : Replace the `sustain` attribute.
    setRelease(x) : Replace the `release` attribute.
    setDur(x) : Replace the `dur` attribute.

    Attributes:
    
    attack : float. Duration of the attack phase in seconds.
    decay : float. Duration of the decay in seconds.
    sustain : float. Amplitude of the sustain phase.
    release : float. Duration of the release in seconds.
    dur : float. Total duration of the envelope.
    
    Notes:

    The out() method is bypassed. Adsr's signal can not be sent to audio outs.
    
    Shape of a classical Adsr:

          -
         -  -
        -     -
       -        ------------------------
      -                                  -
     -                                     -
    -                                        -
      att - dec -        sustain       - rel
     
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> f = Adsr(attack=.01, decay=.2, sustain=.5, release=.1, dur=5, mul=.5)
    >>> a = Noise(mul=f).out()
    >>> f.play()    
    
    """
    def __init__(self, attack=0.01, decay=0.05, sustain=0.707, release=0.1, dur=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._attack = attack
        self._decay = decay
        self._sustain = sustain
        self._release = release
        self._dur = dur
        self._mul = mul
        self._add = add
        attack, decay, sustain, release, dur, mul, add, lmax = convertArgsToLists(attack, decay, sustain, release, dur, mul, add)
        self._base_objs = [Adsr_base(wrap(attack,i), wrap(decay,i), wrap(sustain,i), wrap(release,i), wrap(dur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['attack', 'decay', 'sustain', 'release', 'dur', 'mul', 'add']

    def out(self, chnl=0, inc=1):
        """
        Bypassed. Can't be sent to audio outs.
        """
        pass

    def setAttack(self, x):
        """
        Replace the `attack` attribute.
        
        Parameters:

        x : float
            new `attack` attribute.
        
        """
        self._attack = x
        x, lmax = convertArgsToLists(x)
        [obj.setAttack(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        """
        Replace the `decay` attribute.
        
        Parameters:

        x : float
            new `decay` attribute.
        
        """
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSustain(self, x):
        """
        Replace the `sustain` attribute.
        
        Parameters:

        x : float
            new `sustain` attribute.
        
        """
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setSustain(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setRelease(self, x):
        """
        Replace the `sustain` attribute.
        
        Parameters:

        x : float
            new `sustain` attribute.
        
        """
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setRelease(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.
        
        Parameters:

        x : float
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

    @property
    def attack(self):
        """float. Duration of the attack phase in seconds.""" 
        return self._attack
    @attack.setter
    def attack(self, x): self.setAttack(x)

    @property
    def decay(self):
        """float. Duration of the decay phase in seconds.""" 
        return self._decay
    @decay.setter
    def decay(self, x): self.setDecay(x)

    @property
    def sustain(self):
        """float. Amplitude of the sustain phase.""" 
        return self._sustain
    @sustain.setter
    def sustain(self, x): self.setSustain(x)

    @property
    def release(self):
        """float. Duration of the release phase in seconds.""" 
        return self._release
    @release.setter
    def release(self, x): self.setRelease(x)

    @property
    def dur(self):
        """float. Total duration of the envelope.""" 
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

class Linseg(PyoObject):
    """
    Trace a series of line segments between specified break-points. 
    
    The play() method starts the envelope and is not called at the 
    object creation time.
    
    Parent class: PyoObject

    Parameters:

    list : list of tuples
        Points used to construct the line segments. Each tuple is a
        new point in the form (time, value). Times are given in seconds
        and must be in increasing order.
    loop : boolean, optional
        Looping mode. Defaults to False.
        
    Methods:

    setList(x) : Replace the `list` attribute.
    setLoop(x) : Replace the `loop` attribute.

    Attributes:
    
    list : list of tuples. Points used to construct the line segments.
    loop : boolean. Looping mode.
    
    Notes:

    The out() method is bypassed. Linseg's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> l = Linseg([(0,500),(.03,1000),(.1,700),(1,500),(2,500)], loop=True)
    >>> a = Sine(freq=l, mul=.5).out()
    >>> # then call:
    >>> l.play()
    
    """
    def __init__(self, list, loop=False, mul=1, add=0):
        PyoObject.__init__(self)
        self._list = list
        self._loop = loop
        self._mul = mul
        self._add = add
        loop, mul, add, lmax = convertArgsToLists(loop, mul, add)
        self._base_objs = [Linseg_base(list, wrap(loop,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['list', 'loop', 'mul', 'add']

    def out(self, chnl=0, inc=1):
        pass

    def setList(self, x):
        """
        Replace the `list` attribute.
        
        Parameters:

        x : list of tuples
            new `list` attribute.
        
        """
        self._list = x
        [obj.setList(x) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        """
        Replace the `loop` attribute.
        
        Parameters:

        x : boolean
            new `loop` attribute.
        
        """
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

    @property
    def list(self):
        """float. List of points (time, value).""" 
        return self._list
    @list.setter
    def list(self, x): self.setList(x)

    @property
    def loop(self):
        """boolean. Looping mode.""" 
        return self._loop
    @loop.setter
    def loop(self, x): self.setLoop(x)

class Expseg(PyoObject):
    """
    Trace a series of exponential segments between specified break-points. 

    The play() method starts the envelope and is not called at the 
    object creation time.

    Parent class: PyoObject

    Parameters:

    list : list of tuples
        Points used to construct the line segments. Each tuple is a
        new point in the form (time, value). Times are given in seconds
        and must be in increasing order.
    loop : boolean, optional
        Looping mode. Defaults to False.
    exp : float, optional
        Exponent factor. Used to control the slope of the curves.
        Defaults to 10.
    inverse : boolean, optional
        If True, downward slope will be inversed. Useful to create 
        biexponential curves. Defaults to True.

    Methods:

    setList(x) : Replace the `list` attribute.
    setLoop(x) : Replace the `loop` attribute.
    setExp(x) : Replace the `exp` attribute.
    setInverse(x) : Replace the `inverse` attribute.

    Attributes:

    list : list of tuples. Points used to construct the line segments.
    loop : boolean. Looping mode.
    exp : float. Exponent factor.    
    inverse : boolean. Inversion of downward slope.

    Notes:

    The out() method is bypassed. Expseg's signal can not be sent to audio outs.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> l = Expseg([(0,500),(.03,1000),(.1,700),(1,500),(2,500)], loop=True)
    >>> a = Sine(freq=l, mul=.5).out()
    >>> # then call:
    >>> l.play()

    """
    def __init__(self, list, loop=False, exp=10, inverse=True, mul=1, add=0):
        PyoObject.__init__(self)
        self._list = list
        self._loop = loop
        self._exp = exp
        self._inverse = inverse
        self._mul = mul
        self._add = add
        loop, exp, inverse, mul, add, lmax = convertArgsToLists(loop, exp, inverse, mul, add)
        self._base_objs = [Expseg_base(list, wrap(loop,i), wrap(exp,i), wrap(inverse,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['list', 'loop', 'exp', 'inverse', 'mul', 'add']

    def out(self, chnl=0, inc=1):
        pass

    def setList(self, x):
        """
        Replace the `list` attribute.

        Parameters:

        x : list of tuples
            new `list` attribute.

        """
        self._list = x
        [obj.setList(x) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        """
        Replace the `loop` attribute.

        Parameters:

        x : boolean
            new `loop` attribute.

        """
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        """
        Replace the `exp` attribute.

        Parameters:

        x : float
            new `exp` attribute.

        """
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInverse(self, x):
        """
        Replace the `inverse` attribute.

        Parameters:

        x : boolean
            new `inverse` attribute.

        """
        self._inverse = x
        x, lmax = convertArgsToLists(x)
        [obj.setInverse(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

    @property
    def list(self):
        """float. List of points (time, value).""" 
        return self._list
    @list.setter
    def list(self, x): self.setList(x)

    @property
    def loop(self):
        """boolean. Looping mode.""" 
        return self._loop
    @loop.setter
    def loop(self, x): self.setLoop(x)

    @property
    def exp(self):
        """float. Exponent factor.""" 
        return self._exp
    @exp.setter
    def exp(self, x): self.setExp(x)

    @property
    def inverse(self):
        """boolean. Inverse downward slope.""" 
        return self._inverse
    @inverse.setter
    def inverse(self, x): self.setInverse(x)

class SigTo(PyoObject):
    """
    Convert numeric value to PyoObject signal with portamento.
    
    When `value` attribute is changed, a ramp is applied from the
    current value to the new value.
    
    Parent class: PyoObject

    Parameters:

    value : float
        Numerical value to convert.
    time : float, optional
        Ramp time, in seconds, to reach the new value. Defaults to 0.025.
    init : float, optional
        Initial value of the internal memory. Defaults to 0.

    Methods:

    setValue(x) : Changes the value of the signal stream.
    setTime(x) : Changes the ramp time.
    
    Attributes:
    
    value : float. Numerical value to convert.
    time : float. Ramp time.
    
    Notes:

    The out() method is bypassed. SigTo's signal can not be sent to audio outs.
    
    Examples:
    
    >>> s = Server().boot()
    >>> fr = SigTo(value=400, time=.5, init=400)
    >>> a = Sine(freq=fr, mul=.5).out()
    >>> s.start()
    >>> fr.value = 800

    """
    def __init__(self, value, time=0.025, init=0.0, mul=1, add=0):
        PyoObject.__init__(self)
        self._value = value
        self._time = time
        self._mul = mul
        self._add = add
        value, time, init, mul ,add, lmax = convertArgsToLists(value, time, init, mul, add)
        self._base_objs = [SigTo_base(wrap(value,i), wrap(time,i), wrap(init,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['value', 'time', 'mul', 'add']

    def setValue(self, x):
        """
        Changes the value of the signal stream.

        Parameters:

        x : float
            Numerical value to convert.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        """
        Changes the ramp time of the object.

        Parameters:

        x : float
            New ramp time.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
    
    @property
    def value(self):
        """float. Numerical value to convert.""" 
        return self._value
    @value.setter
    def value(self, x): self.setValue(x)    

    @property
    def time(self):
        """float. Ramp time.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)    
