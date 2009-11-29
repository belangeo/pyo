from _core import *

######################################################################
### Controls
######################################################################                                       
class Fader(PyoObject):
    def __init__(self, fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0):
        self._fadein = fadein
        self._fadeout = fadeout
        self._dur = dur
        self._mul = mul
        self._add = add
        fadein, fadeout, dur, mul, add, lmax = convertArgsToLists(fadein, fadeout, dur, mul, add)
        self._base_objs = [Fader_base(wrap(fadein,i), wrap(fadeout,i), wrap(dur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0):
        pass

    def setFadein(self, x):
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def fadein(self): return self._fadein
    @property
    def fadeout(self): return self._fadeout
    @property
    def dur(self): return self._dur
    @fadein.setter
    def fadein(self, x): self.setFadein(x)
    @fadeout.setter
    def fadeout(self, x): self.setFadeout(x)
    @dur.setter
    def dur(self, x): self.setDur(x)

class Port(PyoObject):
    def __init__(self, input, risetime=0.05, falltime=0.05, mul=1, add=0):
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, mul, add)
        self._base_objs = [Port_base(wrap(in_fader,i), wrap(risetime,i), wrap(falltime,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setRiseTime(self, x):
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def risetime(self): return self._risetime
    @risetime.setter
    def risetime(self, x): self.setRiseTime(x)
    @property
    def falltime(self): return self._falltime
    @falltime.setter
    def falltime(self, x): self.setFallTime(x)
