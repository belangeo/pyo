from _core import *

######################################################################
### Tables
######################################################################                                       
class HarmTable(PyoTableObject):
    def __init__(self, list=[1.], size=8192):
        self._size = size
        self._base_objs = [HarmTable_base(list, size)]
        
    def setSize(self, size):
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def getSize(self):
        return self._size
        
    def replace(self, list):        
        [obj.replace(list) for obj in self._base_objs]

    @property
    def size(self): return self._size
    @size.setter
    def size(self, x): self.setSize(x)
        
class HannTable(PyoTableObject):
    def __init__(self, size=8192):
        self._size = size
        self._base_objs = [HannTable_base(size)]

    def setSize(self, size):
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def getSize(self): 
        return self._size

    @property
    def size(self): return self._size
    @size.setter
    def size(self, x): self.setSize(x)

class SndTable(PyoTableObject):
    def __init__(self, path, chnl=None):
        self._snd_size, self._snd_sr, self._snd_chnls = sndinfo(path)
        if chnl == None:
            self._base_objs = [SndTable_base(path, i) for i in range(self._snd_chnls)]
        else:
            self._base_objs = [SndTable_base(path, chnl)]
                
    def getSize(self):
        return self._snd_size
        
    def getRate(self):
        return self._base_objs[0].getRate()

class NewTable(PyoTableObject):
    def __init__(self, length, chnls=1):
        self._base_objs = [NewTable_base(length) for i in range(chnls)]
                
    def getSize(self):
        return self._base_objs[0].getSize()

    def getLength(self):
        return self._base_objs[0].getLength()
             
    def getRate(self):
        return self._base_objs[0].getRate()

class TableRec(PyoObject):
    def __init__(self, input, table, fadetime=0):
        self._input - input
        self._table = table
        self._in_fader = InputFader(input)
        in_fader, table, fadetime, lmax = convertArgsToLists(self._in_fader, table, fadetime)
        self._base_objs = [TableRec_base(wrap(in_fader,i), wrap(table,i), wrap(fadetime,i)) for i in range(len(table))]

    def out(self, chnl=0):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)
      
    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
