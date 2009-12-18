from _core import *

class Pan(PyoObject):
    """
    Panner.
    
    **Parameters**
    
    input : PyoObject
        Input signal to filter.

    **Methods**
    
    setInput(x, fadetime) : Replace the `input` attribute.

    """
    def __init__(self, input, outs=2, pan=0.5, spread=0.5, mul=1, add=0):
        self._input = input
        self._pan = pan
        self._spread = spread
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, pan, spread, mul, add, lmax = convertArgsToLists(self._in_fader, pan, spread, mul, add)
        self._base_players = [Panner_base(wrap(in_fader,i), outs, wrap(pan,i), wrap(spread,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(outs):
                self._base_objs.append(Pan_base(wrap(self._base_players,i), j, wrap(mul,i), wrap(add,i)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj
     
    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        **Parameters**

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setPan(self, x):
        """Replace the `pan` attribute.
        
        **Parameters**

        x : float or PyoObject
            new `pan` attribute.
        
        """
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setSpread(self, x):
        """Replace the `spread` attribute.
        
        **Parameters**

        x : float or PyoObject
            new `spread` attribute.
        
        """
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x,i)) for i, obj in enumerate(self._base_players)]
                     
    def play(self):
        self._base_players = [obj.play() for obj in self._base_players]
        self._base_objs = [obj.play() for obj in self._base_objs]
        return self

    def out(self, chnl=0, inc=1):
        self._base_players = [obj.play() for obj in self._base_players]
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
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x) 
    @property
    def pan(self): return self._pan
    @pan.setter
    def pan(self, x): self.setPan(x) 
    @property
    def spread(self): return self._spread
    @spread.setter
    def spread(self, x): self.setSpread(x) 
