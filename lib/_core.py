# -*- coding: utf-8 -*-
from types import ListType
from _pyo import *

######################################################################
### Utilities
######################################################################
def convertArgsToLists(*args):
    first = True
    for i in args:
        if isinstance(i, PyoObject): pass  
        elif isinstance(i, PyoTableObject): pass 
        elif type(i) != ListType: i = [i]
            
        if first: tup = (i,)
        else: tup = tup + (i,)
        
        first = False
        
    lengths = [len(i) for i in tup]
    max_length = max(lengths)
    tup = tup + (max_length, )  
    return tup

def wrap(arg, i):
    x = arg[i % len(arg)]
    if isinstance(x, PyoObject):
        return x[0]
    else:
        return x

######################################################################
### PyoObject -> base class for pyo sound objects
######################################################################
class PyoObject(object):
    def __init__(self):
        pass

    def __add__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([obj + wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._add_dummy
        
    def __radd__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([obj + wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._add_dummy
            
    def __iadd__(self, x):
        self.setAdd(x)
        return self

    def __sub__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([obj - wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._add_dummy

    def __rsub__(self, x):
        x, lmax = convertArgsToLists(x)
        self._add_dummy = Dummy([Sig(wrap(x,i)) - obj for i, obj in enumerate(self._base_objs)])
        return self._add_dummy

    def __isub__(self, x):
        self.setSub(x)
        return self
 
    def __mul__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([obj * wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy
        
    def __rmul__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([obj * wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy
            
    def __imul__(self, x):
        self.setMul(x)
        return self
 
    def __div__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([obj / wrap(x,i) for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy

    def __rdiv__(self, x):
        x, lmax = convertArgsToLists(x)
        self._mul_dummy = Dummy([Sig(wrap(x,i)) / obj for i, obj in enumerate(self._base_objs)])
        return self._mul_dummy

    def __idiv__(self, x):
        self.setDiv(x)
        return self
        
    def __getitem__(self, i):
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
 
    def __len__(self):
        return len(self._base_objs)

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        
    def getBaseObjects(self):
        return self._base_objs
        
    def play(self):
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0):
        self._base_objs = [obj.out(chnl+i) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_objs]

    def mix(self, voices=1):
        self._mix = Mix(self, voices)
        return self._mix
        
    def setMul(self, x):
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setMul(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setAdd(self, x):
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setAdd(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSub(self, x):
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setSub(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDiv(self, x):
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setDiv(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def mul(self): return self._mul
    @property
    def add(self): return self._add
    @mul.setter
    def mul(self, x): self.setMul(x)
    @add.setter
    def add(self, x): self.setAdd(x)
            
######################################################################
### PyoTableObject -> base class for pyo table objects
######################################################################
class PyoTableObject(object):
    def __init__(self):
        pass

    def __getitem__(self, i):
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
 
    def __len__(self):
        return len(self._base_objs)

    def getBaseObjects(self):
        return self._base_objs

######################################################################
### Internal classes -> Used by pyo
######################################################################
class Mix(PyoObject):
    def __init__(self, input, voices=1, mul=1, add=0):
        self._input = input
        self._mul = mul
        self._add = add
        input_objs = input.getBaseObjects()
        if voices < 1: voices = 1
        elif voices > len(input_objs): voices = len(input_objs)
        sub_lists = [[]] * voices
        [sub_lists[i % voices].append(obj) for i, obj in enumerate(input_objs)]
        self._base_objs = [Mix_base(l) for l in sub_lists]
        
class Dummy(PyoObject):
    def __init__(self, objs_list):
        self._base_objs = objs_list

class InputFader(PyoObject):
    def __init__(self, input):
        self._input = input
        input, lmax = convertArgsToLists(input)
        self._base_objs = [InputFader_base(wrap(input,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x,i), fadetime) for i, obj in enumerate(self._base_objs)]

class Sig(PyoObject):
    def __init__(self, value, mul=1, add=0):
        self._value = value
        self._mul = mul
        self._add = add
        value, mul ,add, lmax = convertArgsToLists(value, mul, add)
        self._base_objs = [Sig_base(wrap(value,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setValue(self, x):
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
    
    @property
    def value(self): return self._value
    @value.setter
    def value(self, x): self.setValue(x)    
        
        