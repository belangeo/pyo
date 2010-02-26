from _core import *

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
        print "There is no control on Fader object."

    def demo():
        execfile(DEMOS_PATH + "/Fader_demo.py")
    demo = Call_example(demo)

    def args():
        return("Fader(fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0)")
    args = Print_args(args)

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
        print "There is no control on Fader object."

    #def demo():
    #    execfile(DEMOS_PATH + "/Fader_demo.py")
    #demo = Call_example(demo)

    def args():
        return("Adsr(attack=0.01, decay=0.05, sustain=0.707, release=0.1, dur=0, mul=1, add=0)")
    args = Print_args(args)

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

