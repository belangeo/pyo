# -*- coding: utf-8 -*-
from types import ListType
import random
from _pyo import *

######################################################################
### Utilities
######################################################################
class Call_example:
    def __init__(self, callback):
        self.__call__ = callback
        
def convertArgsToLists(*args):
    """
    Convert all arguments to list if not already a list or a PyoObject. 
    Return new args and maximum list length.
    
    """
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
    """
    Return value a position `i` from `arg` with wrap around `arg` length.
    
    """
    x = arg[i % len(arg)]
    if isinstance(x, PyoObject):
        return x[0]
    else:
        return x

######################################################################
### PyoObject -> base class for pyo sound objects
######################################################################
class PyoObject(object):
    """
    Base class for all pyo objects that manipulate vectors of samples.
    
    User should never instantiates an object of this class.

    **Attributes**

    mul : float or PyoObject
        Multiplication factor.
    add : float or PyoObject
        Addition factor.

    **Methods**

    play() : Start processing without sending samples to output. This method is called automatically
        at the object creation.
    stop() : Stop processing.
    out(chnl, inc) : Start processing and send samples to audio output beginning at `chnl`.
    mix(voices) : Mix object's audio streams into `voices` streams and return the Mix object.
    setMul(x) : Replace the `mul` attribute.
    setAdd(x) : Replace the `add` attribute.
    setDiv(x) : Replace and inverse the `mul` attribute.
    setSub(x) : Replace and inverse the `add` attribute.
    
    **Notes**

    Other operations :
    
    len(obj) : Return the number of audio streams in an object.
    obj[x] : Return stream `x` of the object. `x` is a number from 0 to len(obj) - 1.
    del obj : Perform a clean delete of the object.
    
    Arithmetics :
    
    Multiplication, addition, division and substraction can be applied between pyo objects
    or between pyo object and number. Return a Dummy object with the result of the operation.
    `b = a * 0.5` creates a Dummy object `b` with `mul` attribute set to 0.5 and leave `a` untouched.
    
    Inplace multiplication, addition, division and substraction can be applied between pyo 
    objects or between pyo object and number. These operations will replace the `mul` or `add`
    factor of the object. `a *= 0.5` replace `mul` attribute of `a`.
    
    """
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
        """Return a list of audio Stream objects."""
        return self._base_objs
        
    def play(self):
        """Start processing without sending samples to output. This method is called automatically
        at the object creation."""
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        """
        Start processing and send samples to audio output beginning at `chnl`.
        
        **Parameters**

        chnl : int, optional
            Physical output assigned to the first audio stream of the object. Default to 0.

            If `chnl` is an integer equal or greater than 0, then successive streams 
            increment output number by `inc` and wrap around the global number of channels.
            
            If `chnl` is a negative integer, the streams begin at 0 and increment output 
            number by `inc` and wrap around the global number of channels. Then, the list
            of streams is scrambled.
            
            If `chnl` is a list, successive values in the list will be assigned to successive streams.
            
        inc : int, optional
            Output increment value.
        
        """
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(self._base_objs)]
                self._base_objs = random.sample(self._base_objs, len(self._base_objs))
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        """Stop processing."""
        [obj.stop() for obj in self._base_objs]

    def mix(self, voices=1):
        """Mix object's audio streams into `voices` streams and return the Mix object.
        
        **Parameters**

        voices : int, optional
            Number of audio streams of the Mix object created by this method. If more
            than 1, object's streams are alternated and added into Mix object's streams. 
            Default to 1.
            
        """
        self._mix = Mix(self, voices)
        return self._mix
        
    def setMul(self, x):
        """Replace the `mul` attribute.
        
        **Parameters**

        x : float or PyoObject
            New `mul` attribute.
        
        """
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setMul(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setAdd(self, x):
        """Replace the `add` attribute.
                
        **Parameters**

        x : float or PyoObject
            New `add` attribute.
        
        """
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setAdd(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSub(self, x):
        """Replace and inverse the `mul` attribute.
        
        **Parameters**

        x : float or PyoObject
            New inversed `mul` attribute.
        
        """
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setSub(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDiv(self, x):
        """Replace and inverse the `add` attribute.
                
        **Parameters**

        x : float or PyoObject
            New inversed `add` attribute.
        
        """
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
    """
    Base class for all pyo table objects. A table object object is a buffer memory
    to store precomputed samples. 
    
    User should never instantiates an object of this class.
 
    Operations allowed on all table objects :
    
    len(obj) : Return the number of table streams in an object.
    obj[x] : Return table stream `x` of the object. `x` is a number from 0 to len(obj) - 1.

    """
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
        """Return a list of table Stream objects."""
        return self._base_objs

    def getSize(self):
        """
        Return table size in samples.
        
        """
        return self._size
   
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
        
        