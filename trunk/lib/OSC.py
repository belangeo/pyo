from _core import *

######################################################################
### Open Sound Control
######################################################################                                       
class OscSend(PyoObject):
    def __init__(self, input, port, address, host="127.0.0.1"):    
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, port, address, host, lmax = convertArgsToLists(self._in_fader, port, address, host)
        self._base_objs = [OscSend_base(wrap(in_fader,i), wrap(port,i), wrap(address,i), wrap(host,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)
            
    def out(self, chnl=0):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
         
class OscReceive(PyoObject):
    def __init__(self, port, address, mul=1, add=0):    
        self._mul = mul
        self._add = add
        address, mul, add, lmax = convertArgsToLists(address, mul, add)
        self._address = address
        self._mainReceiver = OscReceiver_base(port, address)
        self._base_objs = [OscReceive_base(self._mainReceiver, wrap(address,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __getitem__(self, i):
        if type(i) == type(''):
            return self._base_objs[self._address.index(i)]
        elif i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
             
    def out(self, chnl=0):
        pass
        