from _core import *

class Pan(PyoObject):
    """
    Cosinus panner with control on the spread factor.

    Parent class: PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to process.
    outs : int, optional
        Number of channels on the panning circle. Defaults to 2.
    pan : float or PyoObject
        Position of the sound on the panning circle, between 0 and 1. 
        Defaults to 0.5.
    spread : float or PyoObject
        Amount of sound leaking to the surrounding channels, 
        between 0 and 1. Defaults to 0.5.
 
    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    setPan(x) : Replace the `pan` attribute.
    setSpread(x) : Replace the `spread` attribute.

    Attributes:
    
    input : PyoObject. Input signal to process.
    pan : float or PyoObject. Position of the sound on the panning circle.
    spread : float or PyoObject. Amount of sound leaking to the 
        surrounding channels. 

    Examples:
    
    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = Noise(mul=.5)
    >>> lfo = Sine(freq=1, mul=.5, add=.5)
    >>> p = Pan(a, outs=2, pan=lfo).out()
    
    """ 
    def __init__(self, input, outs=2, pan=0.5, spread=0.5, mul=1, add=0):
        self._input = input
        self._pan = pan
        self._outs = outs
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

    def __dir__(self):
        return ['input', 'pan', 'spread', 'mul', 'add']

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
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setPan(self, x):
        """
        Replace the `pan` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pan` attribute.
        
        """
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setSpread(self, x):
        """
        Replace the `spread` attribute.
        
        Parameters:

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
                self._base_objs = [obj.out(((i*inc) % self._outs)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+((i*inc) % self._outs)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapPan(self._pan),
                        SLMap(0., 1., 'lin', 'spread', self._spread),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def args():
        return('Pan(input, outs=2, pan=0.5, spread=0.5, mul=1, add=0)')
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
 
    @property
    def pan(self):
        """float or PyoObject. Position of the sound on the panning circle."""
        return self._pan
    @pan.setter
    def pan(self, x): self.setPan(x) 

    @property
    def spread(self):
        """float or PyoObject. Amount of sound leaking to the surrounding channels.""" 
        return self._spread
    @spread.setter
    def spread(self, x): self.setSpread(x)

class SPan(PyoObject):
    """
    Simple equal power panner.
    
    Parent class: PyoObject
     
    Parameters:
    
    input : PyoObject
        Input signal to process.
    outs : int, optional
        Number of channels on the panning circle. Defaults to 2.
    pan : float or PyoObject
        Position of the sound on the panning circle, between 0 and 1. 
        Defaults to 0.5.

    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    setPan(x) : Replace the `pan` attribute.

    Attributes:
    
    input : PyoObject. Input signal to process.
    pan : float or PyoObject. Position of the sound on the panning circle.

    Examples:
    
    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = Noise(mul=.5)
    >>> lfo = Sine(freq=1, mul=.5, add=.5)
    >>> p = SPan(a, outs=2, pan=lfo).out()

    """
    def __init__(self, input, outs=2, pan=0.5, mul=1, add=0):
        self._input = input
        self._outs = outs
        self._pan = pan
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, pan, mul, add, lmax = convertArgsToLists(self._in_fader, pan, mul, add)
        self._base_players = [SPanner_base(wrap(in_fader,i), outs, wrap(pan,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(outs):
                self._base_objs.append(SPan_base(wrap(self._base_players,i), j, wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['input', 'pan', 'mul', 'add']

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
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setPan(self, x):
        """
        Replace the `pan` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pan` attribute.
        
        """
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x,i)) for i, obj in enumerate(self._base_players)]
                     
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
                self._base_objs = [obj.out(((i*inc) % self._outs)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+((i*inc) % self._outs)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMapPan(self._pan),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    def args():
        return('SPan(input, outs=2, pan=0.5, mul=1, add=0)')
    args = Print_args(args)

    @property
    def input(self): 
        """PyoObject. Input signal to process."""
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def pan(self): 
        """float or PyoObject. Position of the sound on the panning circle."""
        return self._pan
    @pan.setter
    def pan(self, x): self.setPan(x)

