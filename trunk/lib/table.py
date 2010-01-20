from _core import *

######################################################################
### Tables
######################################################################                                       
class HarmTable(PyoTableObject):
    """
    Generate composite waveforms made up of weighted sums of simple sinusoids.
    
    Parameters:
    
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers 1,2,3, etc. Defaults to [1].
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table. This will erase the previously drawn waveform.
    replace(list) : Redraw the waveform according to the new `list` parameter.
    
    Attributes:
    
    list : list, optional
        Relative strengths of the fixed harmonic partial numbers.
    size : int, optional
        Table size in samples.

    """
    def __init__(self, list=[1., 0.], size=8192):
        self._list = list
        self._size = size
        self._base_objs = [HarmTable_base(list, size)]
        
    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously drawn waveform.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics relative strengths.
        
        Parameters:
        
        list : list
            Relative strengths of the fixed harmonic partial numbers 1,2,3, etc.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    #def demo():
    #    execfile("demos/HarmTable_demo.py")
    #demo = Call_example(demo)

    def args():
        return('HarmTable(list=[1.], size=8192)')
    args = Print_args(args)

    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def list(self): 
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list
    @list.setter
    def list(self, x): self.replace(x)
    
class HannTable(PyoTableObject):
    """
    Generate Hanning window. 
    
    Parameters:
    
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table. This will redraw the envelope.
    
    Attributes:
    
    size : int
        Table size in samples.

    """
    def __init__(self, size=8192):
        self._size = size
        self._base_objs = [HannTable_base(size)]

    def setSize(self, size):
        """
        Change the size of the table. This will redraw the envelope.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]

    #def demo():
    #    execfile("demos/HannTable_demo.py")
    #demo = Call_example(demo)

    def args():
        return('HannTable(size=8192)')
    args = Print_args(args)

    @property
    def size(self): 
        """int. Table size in samples."""
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

class LinTable(PyoTableObject):
    """
    Construct a table from segments of straight lines in breakpoint fashion.
    
    Parameters:
    
    list : list, optional
        List of tuples indicating location and value of each points in the table. 
        The default, [(0,0.), (8191, 1.)], creates a straight line from 0.0 at location 0
        to 1.0 at the end of the table (size - 1). Location must be an integer.
    size : int, optional
        Table size in samples. Defaults to 8192.
        
    Methods:
    
    setSize(size) : Change the size of the table and rescale the envelope.
    replace(list) : Draw a new envelope according to the new `list` parameter.

    Notes:
    
    Locations in the list must be in increasing order. If the last value is less 
    than size, then the rest will be set to zero. 
    
    Attributes:
    
    list : list
        List of tuples [(location, value), ...].
    size : int, optional
        Table size in samples.

    """
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        self._size = size
        self._base_objs = [LinTable_base(list, size)]
        
    def setSize(self, size):
        """
        Change the size of the table and rescale the envelope.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
    
    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.
        
        Parameters:
        
        list : list
            List of tuples indicating location and value of each points in the table. 
            Location must be integer.

        """      
        self._list = list
        [obj.replace(list) for obj in self._base_objs]

    def getPoints(self):
        return self._base_objs[0].getPoints()

    #def demo():
    #    execfile("demos/LinTable_demo.py")
    #demo = Call_example(demo)

    def args():
        return('LinTable(list=[(0, 0.), (8191, 1.)], size=8192)')
    args = Print_args(args)
        
    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table.""" 
        return self.getPoints()
    @list.setter
    def list(self, x): self.replace(x)

class SndTable(PyoTableObject):
    """
    Load data from a soundfile into a function table.
    
    If `chnl` is None, the table will contain as many sub tables as necessary 
    to read all channels of the loaded sound.
    
    Parameters:
    
    path : string
        Full path name of the sound.
    chnl : int, optional
        Channel number to read in. The default (None) reads all channels.

    Methods:

    setSound(path) : Load a new sound in the table.
    getRate() : Return the frequency in cps at which the sound will be read 
    at its original pitch.

    """
    def __init__(self, path, chnl=None):
        self._size, self._snd_sr, self._snd_chnls = sndinfo(path)
        self._path = path
        if chnl == None:
            self._base_objs = [SndTable_base(path, i) for i in range(self._snd_chnls)]
        else:
            self._base_objs = [SndTable_base(path, chnl)]

    def setSound(self, path):
        """
        Load a new sound in the table.
        
        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load the 
        same channels many times. If the new sound has more channels, the extra 
        channels will be skipped.
        
        Parameters:
        
        path : string
            Full path of the new sound.

        """
        _size, _snd_sr, _snd_chnls = sndinfo(path)
        self._path = path
        [obj.setSound(path, (i%_snd_chnls)) for i, obj in enumerate(self._base_objs)]
        
    def getRate(self):
        return self._base_objs[0].getRate()

    #def demo():
    #    execfile("demos/SndTable_demo.py")
    #demo = Call_example(demo)

    def args():
        return('SndTable(path, chnl=None)')
    args = Print_args(args)

    @property
    def sound(self):
        """string. Full path of the sound.""" 
        return self._path
    @sound.setter
    def sound(self, x): self.setSound(x)

class NewTable(PyoTableObject):
    """
    Create an empty table ready for recording. See `TableRec` to write samples in 
    the table.
    
    Parameters:
    
    length : float
        Length of the table in seconds.
    chnls : int, optional
        Number of channels that will be handled by the table. Defaults to 1.
        
    Methods:    
    
    getSize() : Return the length of the table in samples.
    getLength() : Return the length of the table in seconds.
    getRate() : Return the frequency (cycle per second) to give an oscillator to read the sound 
        at its original pitch.

    """
    def __init__(self, length, chnls=1):
        self._base_objs = [NewTable_base(length) for i in range(chnls)]
                
    def getSize(self):
        return self._base_objs[0].getSize()

    def getLength(self):
        return self._base_objs[0].getLength()
             
    def getRate(self):
        return self._base_objs[0].getRate()

    #def demo():
    #    execfile("demos/NewTable_demo.py")
    #demo = Call_example(demo)

    def args():
        return('NewTable(length, chnls=1)')
    args = Print_args(args)

class TableRec(PyoObject):
    """
    TableRec is for writing samples into a previously created NewTable. See `NewTable` to create
    an empty table.

    Parameters:

    input : PyoObject
        Audio signal to write in the table.
    table : PyoTableObject
        The table where to write samples.
    fadetime : float, optional
        Fade time at the beginning and the end of the recording in seconds. Defaults to 0.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setTable(x) : Replace the `table` attribute.
    play() : Start the recording at the beginning of the table.
    stop() : Stop the recording. Otherwise, record through the end of the table.

    Notes:

    The out() method is bypassed. TableRec returns no signal.
    
    TableRec has no `mul` and `add` attributes.
    
    """
    def __init__(self, input, table, fadetime=0):
        self._input = input
        self._table = table
        self._in_fader = InputFader(input)
        in_fader, table, fadetime, lmax = convertArgsToLists(self._in_fader, table, fadetime)
        self._base_objs = [TableRec_base(wrap(in_fader,i), wrap(table,i), wrap(fadetime,i)) for i in range(len(table))]

    def out(self, chnl=0, inc=1):
        pass

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

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

    def setTable(self, x):
        """Replace the `table` attribute.
        
        Parameters:

        x : NewTable
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    #def demo():
    #    execfile("demos/TableRec_demo.py")
    #demo = Call_example(demo)

    def args():
        return('TableRec(input, table, fadetime=0)')
    args = Print_args(args)
      
    @property
    def input(self):
        """PyoObject. Audio signal to write in the table.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def table(self):
        """PyoTableObject. The table where to write samples."""
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)


class Granulator(PyoObject):
    """
    Granular synthesis.

    Parent class: PyoObject
    
    Parameters:

    table : PyoTableObject
        Table containing the waveform samples.
    env : PyoTableObject
        Table containing the grain envelope.
    pitch : float or PyoObject, optional
        Overall pitch of the granulator. This value transpose the pitch of all grains. Defaults to 1.
    pos : float or PyoObject, optional
        Pointer position, in samples, in the waveform table. Each grain sampled the current value of 
        this stream at the beginning of its envelope and hold it until the end of the grain. 
        Defaults to 0.
    dur : float or PyoObject, optional
        Duration, in seconds, of the grain. Each grain sampled the current value of this stream at the 
        beginning of its envelope and hold it until the end of the grain. Defaults to 0.1.
    grains : int, optional
        Number of grains. Available only at initialization. Defaults to 8.
    basedur : float, optional
        Base duration used to calculate the speed of the pointer to read the grain at its original pitch. 
        By changing the value of the `dur` parameter, transposition per grain can be generated.
        Defaults to 0.1.
    
    Methods:
    
    setTable(x) : Replace the `table` attribute.
    setEnv(x) : Replace the `env` attribute.
    setPitch(x) : Replace the `pitch` attribute.
    setPos(x) : Replace the `pos` attribute.
    setDur(x) : Replace the `dur` attribute.
    setBaseDur(x) : Replace the `basedur` attribute.
    
    Attributes:
    
    table : PyoTableObject. The table where to write samples.
    env : PyoTableObject. Table containing the grain envelope.
    pitch : float or PyoObject. Overall pitch of the granulator.
    pos : float or PyoObject. Position of the pointer in the sound table.
    dur : float or PyoObject. Duration, in seconds, of the grain.
    basedur : float. Duration to read the grain at its original pitch.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable("demos/transparent.aif")
    >>> env = HannTable()
    >>> pos = Phasor(snd.getRate()*.25, 0, snd.getSize())
    >>> dur = Noise(.001, .1)
    >>> g = Granulator(table=snd, env=env, pitch=[1, 1.001], pos=pos, dur=dur, grains=24, mul=.1).out()

    """
    def __init__(self, table, env, pitch=1, pos=0, dur=.1, grains=8, basedur=.1, mul=1, add=0):
        self._table = table
        self._env = env
        self._pitch = pitch
        self._pos = pos
        self._dur = dur
        self._grains = grains
        self._basedur = basedur
        self._mul = mul
        self._add = add
        table, env, pitch, pos, dur, grains, basedur, mul, add, lmax = convertArgsToLists(table, env, pitch, 
                                                                        pos, dur, grains, basedur, mul, add)
        self._base_objs = [Granulator_base(wrap(table,i), wrap(env,i), wrap(pitch,i), wrap(pos,i), wrap(dur,i), 
                          wrap(grains,i), wrap(basedur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setTable(self, x):
        """
        Replace the `table` attribute.
        
        Parameters:

        x : PyoTableObject
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.
        
        Parameters:

        x : PyoTableObject
            new `env` attribute.
        
        """
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pitch` attribute.
        
        """
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pos` attribute.
        
        """
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.
        
        Parameters:

        x : float or PyoObject
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setBaseDur(self, x):
        """
        Replace the `basedur` attribute.
        
        Parameters:

        x : float
            new `basedur` attribute.
        
        """
        self._basedur = x
        x, lmax = convertArgsToLists(x)
        [obj.setBaseDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    #def demo():
    #    execfile("demos/Granulator_demo.py")
    #demo = Call_example(demo)

    def args():
        return('Granulator(table, env, pitch=1, pos=0, dur=.1, grains=8, basedur=.1, mul=1, add=0)')
    args = Print_args(args)

    @property
    def table(self):
        """PyoTableObject. The table where to write samples."""
        return self._table
    @table.setter
    def table(self, x): self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the grain envelope."""
        return self._env
    @env.setter
    def env(self, x): self.setEnv(x)

    @property
    def pitch(self):
        """float or PyoObject. Overall pitch of the granulator."""
        return self._pitch
    @pitch.setter
    def pitch(self, x): self.setPitch(x)

    @property
    def pos(self):
        """float or PyoObject. Position of the pointer in the sound table."""
        return self._pos
    @pos.setter
    def pos(self, x): self.setPos(x)

    @property
    def dur(self):
        """float or PyoObject. Duration, in seconds, of the grain."""
        return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

    @property
    def basedur(self):
        """float. Duration to read the grain at its original pitch."""
        return self._basedur
    @basedur.setter
    def basedur(self, x): self.setBaseDur(x)

