from _core import *

class Hilbert(PyoObject):
    """
    Hilbert transform.
    
    Parent class : PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.

    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    
    Attributes:
    
    input : PyoObject. Input signal to filter.
    
    Notes:
    
    Real and imaginary parts are two separated set of streams. The user should call :
    
    Hilbert['real'] to retrieve the real part.
    Hilbert['imag'] to retrieve the imaginary part.
    
    Examples:
    
    >>> a = SfPlayer(DEMOS_PATH + "/accord.aif", loop=True).out()
    >>> b = Hilbert(a)
    >>> quad = Sine([250, 500], [0, .25])
    >>> mod1 = b['real'] * quad[0]
    >>> mod2 = b['imag'] * quad[1]
    >>> up = mod1 - mod2
    >>> down = mod1 + mod2
    >>> up.out()

    """
    def __init__(self, input, mul=1, add=0):
        self._real_dummy = None
        self._imag_dummy = None
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, lmax = convertArgsToLists(self._in_fader)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [HilbertMain_base(wrap(in_fader,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax2):
            self._base_objs.append(Hilbert_base(wrap(self._base_players,i), 0, wrap(mul,i), wrap(add,i)))
            self._base_objs.append(Hilbert_base(wrap(self._base_players,i), 1, wrap(mul,i), wrap(add,i)))

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

    def __getitem__(self, str):
        if str == 'real':
            if self._real_dummy == None:
                self._real_dummy = Dummy([obj for i, obj in enumerate(self._base_objs) if (i%2) == 0])
            return self._real_dummy
        if str == 'imag':
            if self._imag_dummy == None:
                self._imag_dummy = Dummy([obj for i, obj in enumerate(self._base_objs) if (i%2) == 1])
            return self._imag_dummy
     
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
                self._base_objs = [obj.out(i*inc) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def demo():
        execfile(DEMOS_PATH + "/Hilbert_demo.py")
    demo = Call_example(demo)

    def args():
        return("Hilbert(input, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
