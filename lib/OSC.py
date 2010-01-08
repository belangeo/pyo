from _core import *

######################################################################
### Open Sound Control
######################################################################                                       
class OscSend(PyoObject):
    """
    Sends values over a network via the Open Sound Control protocol.
    
    Uses the OSC protocol to share values to other softwares or other computers.
    Only the first value of each input buffersize will be sent on the OSC port.
    
    **Parameters**
    
    input : PyoObject
        Input signal.
    port : int
        Port on which values are sent. Receiver should listen on the same port.
    address : string
        Address used on the port to identify values. Address is in the form 
        of a Unix path (ex.: '/pitch').
    host : string, optional
        IP address of the target computer. The default, '127.0.0.1', is the localhost.

    **Methods**

    setInput(x, fadetime) : Replace the `input` attribute.

    **Notes**

    The out() method is bypassed. OscSend's signal can't be sent to audio outs.
    
    OscSend has no `mul` and `add` attributes.
    
    """
    def __init__(self, input, port, address, host="127.0.0.1"):    
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, port, address, host, lmax = convertArgsToLists(self._in_fader, port, address, host)
        self._base_objs = [OscSend_base(wrap(in_fader,i), wrap(port,i), wrap(address,i), wrap(host,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        self._input = x
        self._in_fader.setInput(x, fadetime)
            
    def out(self, chnl=0, inc=1):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    #def demo():
    #    execfile("demos/OscSend_demo.py")
    #demo = Call_example(demo)

    def args():
        return('OscSend(input, port, address, host="127.0.0.1")')
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
         
class OscReceive(PyoObject):
    """
    Receives values over a network via the Open Sound Control protocol.
    
    Uses the OSC protocol to receive values from other softwares or other computers.
    Gets a value at the beginning of each buffersize and fill it's buffer with it.
    
    **Parameters**
    
    port : int
        Port on which values are received. Sender should output on the same port.
    address : string
        Address used on the port to identify values. Address is in the form 
        of a Unix path (ex.: '/pitch').

    **Notes**
    
    Audio streams are accessed with the `address` string parameter. The user should call :

    OscReceive['/pitch'] to retreive streams named '/pitch'.

    The out() method is bypassed. OscReceive's signal can't be sent to audio outs.

    """

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
             
    def out(self, chnl=0, inc=1):
        pass

    #def demo():
    #    execfile("demos/OscReceive_demo.py")
    #demo = Call_example(demo)

    def args():
        return('OscReceive(port, address, mul=1, add=0)')
    args = Print_args(args)
        
