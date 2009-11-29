from _core import *

######################################################################
### Effects
######################################################################                                       
class Biquad(PyoObject):
    def __init__(self, input, freq=1000, q=1, type=0, mul=1, add=0):
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, type, mul, add)
        self._base_objs = [Biquad_base(wrap(in_fader,i), wrap(freq,i), wrap(q,i), wrap(type,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setFreq(self, x):
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        self.q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)
    @property
    def q(self): return self._q
    @q.setter
    def q(self, x): self.setQ(x)
    @property
    def type(self): return self._type
    @type.setter
    def type(self, x): self.setType(x)

class Disto(PyoObject):
    def __init__(self, input, drive=.75, slope=.5, mul=1, add=0):
        self._input = input
        self._drive = drive
        self._slope = slope
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, drive, slope, mul, add, lmax = convertArgsToLists(self._in_fader, drive, slope, mul, add)
        self._base_objs = [Disto_base(wrap(in_fader,i), wrap(drive,i), wrap(slope,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)
 
    def setDrive(self, x):
        self._drive = x
        x, lmax = convertArgsToLists(x)
        [obj.setDrive(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def drive(self): return self._drive
    @drive.setter
    def drive(self, x): self.setDrive(x)
    @property
    def slope(self): return self._slope
    @slope.setter
    def slope(self, x): self.setSlope(x)

class Delay(PyoObject):
    def __init__(self, input, delay=0, feedback=0, maxdelay=1, mul=1, add=0):
        self._input = input
        self._delay = delay
        self._feedback = feedback
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, delay, feedback, maxdelay, mul, add, lmax = convertArgsToLists(self._in_fader, delay, feedback, maxdelay, mul, add)
        self._base_objs = [Delay_base(wrap(in_fader,i), wrap(delay,i), wrap(feedback,i), wrap(maxdelay,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]
        
    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDelay(self, x):
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x) 
    @property
    def delay(self): return self._delay
    @delay.setter
    def delay(self, x): self.setDelay(x)
    @property
    def feedback(self): return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)
