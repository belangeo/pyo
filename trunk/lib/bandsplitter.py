from _core import *

class BandSplit(PyoObject):
    """
    Split an input signal into multiple frequency bands.
    
    The signal will be filtered into `num` bands between `min` and `max` frequencies.
    Each band will then be assigned to an independent audio stream. Useful for multiband
    processing.
    
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    num : int, optional
        Number of frequency bands created. Initialization time only. Defaults to 6.
    min : float, optional
        Lowest frequency. Initialization time only. Defaults to 20.
    max : float, optional
        Highest frequency. Initialization time only. Defaults to 20000.
    q : float or PyoObject, optional
        Q of the filters, defined as bandwidth/cutoff. Should be between 1 and 500. Defaults to 1.

    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    setQ(x) : Replace the `q` attribute.

    """
    def __init__(self, input, num=6, min=20, max=20000, q=1, mul=1, add=0):
        self._input = input
        self._num = num
        self._min = min
        self._max = max
        self._q = q
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, q, lmax = convertArgsToLists(self._in_fader, q)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [BandSplitter_base(wrap(in_fader,i), num, min, max, wrap(q,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(num):
                self._base_objs.append(BandSplit_base(wrap(self._base_players,i), j, wrap(mul,j), wrap(add,j)))

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
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setQ(self, x):
        """
        Replace the `q` attribute.
        
        Parameters:

        x : float or PyoObject
            new `q` attribute.
        
        """
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x,i)) for i, obj in enumerate(self._base_players)]
                        
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

    def demo():
        execfile("demos/BandSplit_demo.py")
    demo = Call_example(demo)

    def args():
        return("BandSplit(input, num=6, min=20, max=20000, q=1, mul=1, add=0)")
    args = Print_args(args)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def q(self): 
        """float or PyoObject. Q of the filters."""
        return self._q
    @q.setter
    def q(self, x): self.setQ(x) 
