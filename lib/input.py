from _core import *

######################################################################
### Sources
######################################################################                                       
class Sine(PyoObject):
    def __init__(self, freq=1000, phase=0, mul=1, add=0):
        self._freq = freq
        self._phase = phase
        self._mul = mul
        self._add = add
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Sine_base(wrap(freq,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setFreq(self, x):
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setPhase(self, x):
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self): return self._freq
    @property
    def phase(self): return self._phase
    @freq.setter
    def freq(self, x): self.setFreq(x)
    @phase.setter
    def phase(self, x): self.setPhase(x)
 
class Osc(PyoObject):
    def __init__(self, table, freq=1000, mul=1, add=0):
        self._table = table
        self._freq = freq
        self._mul = mul
        self._add = add
        table, freq, mul, add, lmax = convertArgsToLists(table, freq, mul, add)
        self._base_objs = [Osc_base(wrap(table,i), wrap(freq,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setFreq(self, x):
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self): return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

class Input(PyoObject):
    def __init__(self, chnl, mul=1, add=0):                
        self._chnl = chnl
        self._mul = mul
        self._add = add
        chnl, mul, add, lmax = convertArgsToLists(chnl, mul, add)
        self._base_objs = [Input_base(wrap(chnl,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

class Noise(PyoObject):
    def __init__(self, mul=1, add=0):                
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [Noise_base(wrap(mul,i), wrap(add,i)) for i in range(lmax)]
